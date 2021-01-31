#include <cmath>

#include "Extrapolators/EZExtrapolator.h"

//Need to check if the origin can reach the victim!
//Attacker is checked for this, the origin needs to as well

EZExtrapolator::EZExtrapolator(bool random_tiebraking,
                                bool store_invert_results, 
                                bool store_depref_results, 
                                std::string announcement_table,
                                std::string results_table, 
                                std::string inverse_results_table, 
                                std::string depref_results_table, 
                                std::string config_section,
                                std::vector<std::string> *policy_tables, 
                                uint32_t iteration_size,
                                uint32_t num_rounds,
                                uint32_t community_detection_local_threshold,
                                int exclude_as_number,
                                uint32_t mh_mode) : BlockedExtrapolator(random_tiebraking, store_invert_results, store_depref_results, iteration_size, mh_mode) {
    
    graph = new EZASGraph();
    querier = new EZSQLQuerier(announcement_table, results_table, inverse_results_table, depref_results_table, policy_tables, exclude_as_number, config_section);
    communityDetection = new CommunityDetection(this, community_detection_local_threshold);
    
    this->num_rounds = num_rounds;
    this->round = 1;
    this->next_unused_asn = 65434;
}

EZExtrapolator::EZExtrapolator(uint32_t community_detection_local_threshold) : EZExtrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                        ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_POLICY_TABLES, DEFAULT_ITERATION_SIZE, 
                        0, community_detection_local_threshold, -1, DEFAULT_MH_MODE) {

}

EZExtrapolator::EZExtrapolator() 
    : EZExtrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                        ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_POLICY_TABLES, DEFAULT_ITERATION_SIZE, 
                        0, DEFAULT_COMMUNITY_DETECTION_LOCAL_THRESHOLD, -1, DEFAULT_MH_MODE) { }

EZExtrapolator::~EZExtrapolator() {
    delete communityDetection;
}

void EZExtrapolator::init() {
    BlockedExtrapolator::init();
    for (unsigned int i = 1; i <= num_rounds; i++) {
        this->querier->clear_round_results_from_db(i);
        this->querier->create_round_results_tbl(i);
    }

    // Set attacker(s)
    pqxx::result returned_pairs = this->querier->get_attacker_po_pairs();
    for (pqxx::result::size_type i = 0; i < returned_pairs.size(); i++) {
        // Get row origin
        uint32_t origin;
        returned_pairs[i]["origin_hijack_asn"].to(origin);
        // Get row prefix
        std::string ip = returned_pairs[i]["host"].c_str();
        std::string mask = returned_pairs[i]["netmask"].c_str();
        Prefix<> tmp_prefix(ip, mask);
        attacker_prefix_pairs.insert(std::pair<Prefix<>,uint32_t>(tmp_prefix, origin));
    }
}

void EZExtrapolator::gather_community_detection_reports() {
    //Go through all of the ases and see if there are any attacking announcements
    //send them to the community detection
    for(auto &as_pair : *graph->ases) {
        EZAS *as = as_pair.second;

        if(as->policy == EZAS_TYPE_BGP) {
            continue;
        }

        //For all of the announcements, report the path if it was from an attacker. CD will check if they are truly visible
        for(auto &announcement_pair : *as->all_anns) {
            //Non attacking announcement will not have an invalid MAC
            //An attacking announcement will, and community detection will check if it is visible
            if(announcement_pair.second.from_attacker) {
                //Logger::getInstance().log("Debug") << "Announcement with path: " << announcement_search->second.as_path << " is being sent to CD";
                communityDetection->add_report(announcement_pair.second, graph);
            }
        }
    }
}

void EZExtrapolator::perform_propagation() {
    this->init();

    std::cout << "Generating subnet blocks..." << std::endl;
    
    // Generate iteration blocks
    std::vector<Prefix<>*> *prefix_blocks = new std::vector<Prefix<>*>; // Prefix blocks
    std::vector<Prefix<>*> *subnet_blocks = new std::vector<Prefix<>*>; // Subnet blocks
    Prefix<> *cur_prefix = new Prefix<>("0.0.0.0", "0.0.0.0"); // Start at 0.0.0.0/0
    this->populate_blocks(cur_prefix, prefix_blocks, subnet_blocks); // Select blocks based on iteration size
    delete cur_prefix;
    
    // Propagate the graph, but after each round disconnect the attacker from the neighbor on the path
    for(round = 1; round <= num_rounds; round++) {
        std::cout << "Round #" << round << std::endl;
        for (auto &as : *this->graph->ases) {
            as.second->community_detection = communityDetection;
        }
        this->extrapolate(prefix_blocks, subnet_blocks);

        communityDetection->process_reports(graph);

        graph->clear_announcements();
        //Reset memory for the graph so it can calculate ranks, components, etc... accordingly
        graph->reset_ranks_and_components();
        //Re-calculate the components with these new relationships
        graph->process(querier);
    }
    
    // Cleanup
    for (Prefix<> *p : *prefix_blocks) { delete p; }
    for (Prefix<> *p : *subnet_blocks) { delete p; }

    delete prefix_blocks;
    delete subnet_blocks;
}

/*
 * This will seed the announcement at the two different locations: the origin and the attacker
 * The attacker is sending as if it were a provider to the origin AS
 * We also write down what prefix the attcker is targetting
 * 
 * In addition, seeded announcement such as these don't need path propagation since they should not (very unlikely) have an attacker in the path...
 * Attackers are the only announcements that we need paths from, thus we don't need to build up the path as we seed the path
 */
void EZExtrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp /* = 0 */) {
    //BlockedExtrapolator::give_ann_to_as_path(as_path, prefix, timestamp);
    
    uint32_t path_origin_asn = as_path->at(as_path->size() - 1);
    uint32_t attacker_asn = as_path->at(0); // possibly not the attacker_asn

    std::pair<Prefix<>,uint32_t> po = std::pair<Prefix<>,uint32_t>(prefix,attacker_asn);
    auto result = attacker_prefix_pairs.find(po);

    if(result == attacker_prefix_pairs.end()) {
        // Not attacker
        auto as_search = graph->ases->find(path_origin_asn);
        if(as_search == graph->ases->end())
            return;
        
        EZAS* as = as_search->second;

        EZAnnouncement announcement = EZAnnouncement(path_origin_asn, prefix.addr, prefix.netmask, 299, path_origin_asn, timestamp, true, false);
        // Remove first element of as_path so the as doesn't see itself on the path (and reject the announcement because of that)
        as_path->erase(as_path->begin());
        announcement.as_path = *as_path;
        announcement.received_from_asn = NOTHIJACKED;
        as->process_announcement(announcement, this->random_tiebraking);
    } else {
    // Attacker
        //uint32_t victim2_asn = result->second.second;

        //Check if we have a prefix set to attack already, don't announce attack
        //if(graph->victim_to_prefixes->find(victim2_asn) != graph->victim_to_prefixes->end())
        //    return;

        auto attacker_search = graph->ases->find(attacker_asn);
        if(attacker_search == graph->ases->end())
            return;
        
        EZAS* attacker = attacker_search->second;

        //Don't bother with anything of this if the attacker can't show up to the party
        // if(attacker->providers->size() == 0 && attacker->peers->size() == 0)
        //     return;

        //graph->victim_to_prefixes->insert(std::make_pair(victim2_asn, prefix));

        EZAnnouncement attackAnnouncement = EZAnnouncement(path_origin_asn, prefix.addr, prefix.netmask, 300 - as_path->size(), path_origin_asn, timestamp, true, true);
        // Remove first element of as_path so the attacker doesn't see itself on the path (and reject the announcement because of that)
        as_path->erase(as_path->begin());

        /**  Swap out ASNs on the path each round.
          *  
          *  With a fixed k in a k-hop origin hijack, the attacker wants to 
          *  maintain the largest collection of adjacent suspects as possible
          *  without exceeding the threshold. 
          */
        for (unsigned int exp = 0; exp < as_path->size() - 1; exp++) {
            if (round % static_cast<uint32_t>(pow(this->communityDetection->local_threshold, exp)) == 0) {
                (*as_path)[as_path->size()-2-exp] = get_unused_asn();
            }
        }
        // debug
        //for (auto a : *as_path) {
        //    std::cout << a << ',';
        //}
        //std::cout << std::endl;

        attackAnnouncement.as_path = *as_path;
        attackAnnouncement.received_from_asn = HIJACKED;
        attacker->process_announcement(attackAnnouncement, this->random_tiebraking);
    }
}

uint32_t EZExtrapolator::get_unused_asn() {
    // This will start to cause problems near round 1000
    return next_unused_asn--;
}

void EZExtrapolator::save_results(int iteration) {
    this->save_results_round(iteration);
    gather_community_detection_reports();
}

void EZExtrapolator::save_results_round(int iteration) {
    std::stringstream tbl;
    tbl << EZBGPSEC_ROUND_TABLE_BASE_NAME << round;
    // cheeky switcheroo to avoid code duplication
    std::string old_results_table = querier->results_table;
    querier->results_table = tbl.str();
    BaseExtrapolator::save_results(iteration); 
    querier->results_table = old_results_table;
}
