#include <cmath>

#include "Extrapolators/EZExtrapolator.h"

//Need to check if the origin can reach the victim!
//Attacker is checked for this, the origin needs to as well

EZExtrapolator::EZExtrapolator(bool random_tiebraking,
                                bool store_results, 
                                bool store_invert_results, 
                                bool store_depref_results, 
                                std::string announcement_table,
                                std::string results_table, 
                                std::string inverse_results_table, 
                                std::string depref_results_table, 
                                std::string full_path_results_table, 
                                std::vector<std::string> *policy_tables, 
                                std::string config_section,
                                uint32_t iteration_size,
                                uint32_t num_rounds,
                                uint32_t community_detection_local_threshold,
                                int exclude_as_number,
                                uint32_t mh_mode,
                                bool origin_only,
                                std::vector<uint32_t> *full_path_asns,
                                int max_threads) : BlockedExtrapolator(random_tiebraking, store_results, store_invert_results, store_depref_results, 
                                iteration_size, mh_mode, origin_only, full_path_asns, max_threads, false) {
    
    graph = new EZASGraph();
    communityDetection = new CommunityDetection(this, community_detection_local_threshold);
    querier = new EZSQLQuerier(announcement_table, results_table, inverse_results_table, depref_results_table, full_path_results_table, policy_tables, exclude_as_number, config_section);
    
    this->num_rounds = num_rounds;
    this->round = 1;
    this->next_unused_asn = 65434;
}

EZExtrapolator::EZExtrapolator(uint32_t community_detection_local_threshold) : EZExtrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                        ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE,
                        DEFAULT_POLICY_TABLES, DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, 
                        0, community_detection_local_threshold, -1, DEFAULT_MH_MODE, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS) {

}

EZExtrapolator::EZExtrapolator() 
    : EZExtrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS,
                        ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE,
                        DEFAULT_POLICY_TABLES, DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, 
                        0, DEFAULT_COMMUNITY_DETECTION_LOCAL_THRESHOLD, -1, DEFAULT_MH_MODE, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS) { }

EZExtrapolator::~EZExtrapolator() {
    delete communityDetection;
}

void EZExtrapolator::init() {
    BlockedExtrapolator::init();
    for (unsigned int i = 0; i <= num_rounds; i++) {
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
        unsigned int id = atoi(returned_pairs[i]["prefix_id"].c_str());
        unsigned int blk_id = atoi(returned_pairs[i]["block_prefix_id"].c_str());
        Prefix<> tmp_prefix(ip, mask, id, blk_id);
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
        for(auto announcement_pair : *as->all_anns) {
            //Non attacking announcement will not have an invalid MAC
            //An attacking announcement will, and community detection will check if it is visible
            if(announcement_pair.from_attacker) {
                //Logger::getInstance().log("Debug") << "Announcement with path: " << announcement_search->second.as_path << " is being sent to CD";
                communityDetection->add_report(announcement_pair, graph);
            }
        }
    }
}

void EZExtrapolator::perform_propagation() {
    this->init();

    BOOST_LOG_TRIVIAL(info) << "Generating subnet blocks...";
    
    // Generate iteration blocks
    std::vector<Prefix<>*> *prefix_blocks = new std::vector<Prefix<>*>; // Prefix blocks
    std::vector<Prefix<>*> *subnet_blocks = new std::vector<Prefix<>*>; // Subnet blocks
    Prefix<> *cur_prefix = new Prefix<>("0.0.0.0", "0.0.0.0", 0, 0); // Start at 0.0.0.0/0
    this->populate_blocks(cur_prefix, prefix_blocks, subnet_blocks); // Select blocks based on iteration size
    delete cur_prefix;

    // Propagate "round zero" so each AS knows what it can see with regular BGP
    round = 0;
    BOOST_LOG_TRIVIAL(info) << "Pre-populating prev_anns...";
    for (auto &as : *this->graph->ases) {
        as.second->community_detection = communityDetection;
    }
    this->extrapolate(prefix_blocks, subnet_blocks);
    graph->clear_announcements();
    round = 1;
    
    // Propagate the graph, but after each round disconnect the attacker from the neighbor on the path
    for(round = 1; round <= num_rounds; round++) {
        BOOST_LOG_TRIVIAL(info) << "Round #" << round;
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
void EZExtrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp /* = 0 */, uint32_t prefix_id) {
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
        BOOST_LOG_TRIVIAL(info) << "GIVING NON ATTACKER ANN\n";

        Priority pr;
        pr.relationship = 3;
        EZAnnouncement announcement = EZAnnouncement(path_origin_asn, prefix, pr, NOTHIJACKED, timestamp, true, false);
        // Remove first element of as_path so the as doesn't see itself on the path (and reject the announcement because of that)
        as_path->erase(as_path->begin());
        announcement.as_path = *as_path;
        announcement.received_from_asn = NOTHIJACKED;
        as->process_announcement(announcement, this->random_tiebraking);
    } else {
    // Attacker
        BOOST_LOG_TRIVIAL(info) << "GIVING ATTACKER ANN\n";
        //uint32_t victim2_asn = result->second.second;

        //Check if we have a prefix set to attack already, don't announce attack
        //if(graph->victim_to_prefixes->find(victim2_asn) != graph->victim_to_prefixes->end())
        //    return;

        auto attacker_search = graph->ases->find(attacker_asn);
        if(attacker_search == graph->ases->end())
            return;
        EZAS* attacker = attacker_search->second;

        auto as_search = graph->ases->find(path_origin_asn);
        if(as_search == graph->ases->end())
            return;
        EZAS* origin = as_search->second;

        //Don't bother with anything of this if the attacker can't show up to the party
        // if(attacker->providers->size() == 0 && attacker->peers->size() == 0)
        //     return;

        //graph->victim_to_prefixes->insert(std::make_pair(victim2_asn, prefix));
        Priority priority;
        priority.relationship = 2;
        priority.path_length = 1 + as_path->size();

        EZAnnouncement attackAnnouncement = EZAnnouncement(path_origin_asn, prefix, priority, HIJACKED, timestamp, true, true);

        // Remove first element of as_path so the attacker doesn't see itself on the path (and reject the announcement because of that)
        as_path->erase(as_path->begin());

        /**  Swap out ASNs on the path each round.
          *  
          *  With a fixed k in a k-hop origin hijack, the attacker wants to 
          *  maintain the largest collection of adjacent suspects as possible
          *  without exceeding the threshold. 
          *          
          *  Consider a fixed k with local threshold t, the matrix of attack
          *  paths is:
          *
          *  [666, a_11, a_12,... a_1t, origin]
          *  [666, a_21, a_22,... a_2t, origin]
          *  ...
          *  [666, a_k1, a_k2,... a_kt, origin]
          */
        for (unsigned int pl = 0; pl < as_path->size() - 1; pl++) {
            (*as_path)[as_path->size()-2-pl] = 65434 - (communityDetection->local_threshold * pl) - 
                 (((round-1) / static_cast<uint32_t>(pow(communityDetection->local_threshold, pl)) % communityDetection->local_threshold));
        }
        // If transitive BGPsec is the policy, try to find a non-adopting neighbor
        // of the origin (that way a valid signature can be used)
        if (origin->policy == EZAS_TYPE_BGPSEC_TRANSITIVE || origin->policy == EZAS_TYPE_BGPSEC) {
            std::set<uint32_t> neighbors;
            neighbors.insert(attacker->providers->begin(), attacker->providers->end());
            neighbors.insert(attacker->peers->begin(), attacker->peers->end());
            neighbors.insert(attacker->customers->begin(), attacker->customers->end());
            for (uint32_t nbr_asn : neighbors) {
                auto nbr_search = graph->ases->find(nbr_asn);
                if(nbr_search == graph->ases->end())
                    continue;
                EZAS* nbr = nbr_search->second;
                if (nbr->policy != EZAS_TYPE_BGP and nbr->policy != EZAS_TYPE_BGPSEC_TRANSITIVE) {
                    if (as_path->size() > 2) {
                        as_path->rbegin()[1] = nbr_asn;
                        break;
                    }
                }
            }
        }

        // debug
        for (auto a : *as_path) {
            std::cout << a << ',';
        }
        std::cout << std::endl;

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

    // Adopters will now report in their process_announcement function since add_report pushes to a delayed data structure
    //gather_community_detection_reports();
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
