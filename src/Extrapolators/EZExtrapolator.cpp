#include "Extrapolators/EZExtrapolator.h"

EZExtrapolator::EZExtrapolator(bool random /* = true */,
                 bool invert_results /* = true */, 
                 bool store_depref /* = false */, 
                 std::string a /* = ANNOUNCEMENTS_TABLE */,
                 std::string r /* = RESULTS_TABLE */, 
                 std::string i /* = INVERSE_RESULTS_TABLE */, 
                 std::string d /* = DEPREF_RESULTS_TABLE */, 
                 uint32_t iteration_size /* = false */) : BlockedExtrapolator(random, invert_results, store_depref, a, r, i, d, iteration_size) {
    
    graph = new EZASGraph();
    querier = new EZSQLQuerier(a, r, i, d);

    total_attacks = 0;
    successful_attacks = 0;
}

EZExtrapolator::~EZExtrapolator() {

}

void EZExtrapolator::init() {
    BlockedExtrapolator::init();

    total_attacks = 0;
    successful_attacks = 0;
}

void EZExtrapolator::perform_propagation() {
    init();

    std::cout << "Generating subnet blocks..." << std::endl;
    
    // Generate iteration blocks
    std::vector<Prefix<>*> *prefix_blocks = new std::vector<Prefix<>*>; // Prefix blocks
    std::vector<Prefix<>*> *subnet_blocks = new std::vector<Prefix<>*>; // Subnet blocks
    Prefix<> *cur_prefix = new Prefix<>("0.0.0.0", "0.0.0.0"); // Start at 0.0.0.0/0
    this->populate_blocks(cur_prefix, prefix_blocks, subnet_blocks); // Select blocks based on iteration size
    delete cur_prefix;

    propagation_drop_connections_helper(prefix_blocks, subnet_blocks);
    
    // Cleanup
    delete prefix_blocks;
    delete subnet_blocks;
}

void EZExtrapolator::propagation_drop_connections_helper(std::vector<Prefix<>*> *prefix_blocks, std::vector<Prefix<>*> *subnet_blocks) {
    static int round = 0;
    round++;

    total_attacks = 0;
    successful_attacks = 0;

    std::cout << "Round #" << round << std::endl;

    extrapolate(prefix_blocks, subnet_blocks);

    if(successful_attacks > 0) {
        std::cout << "Round #" << round << std::endl;

        std::cout << "Successful Attacks " << successful_attacks << std::endl;
        std::cout << "Total Attacks: " << total_attacks << std::endl;
        std::cout << "Probability: " << (successful_attacks / total_attacks) << std::endl;

        //Disconnect attacker from provider
        //Reset memory for the graph so it can calculate ranks, components, etc... accordingly
        graph->disconnectAttackerEdges();
        graph->clear_announcements();

        for(auto element : *graph->ases) {
            element.second->rank = -1;
            element.second->index = -1;
            element.second->onStack = false;
            element.second->lowlink = 0;
            element.second->visited = false;
            element.second->member_ases->clear();

            if(element.second->inverse_results != NULL) {
                for(auto i : *element.second->inverse_results)
                    delete i.second;

                element.second->inverse_results->clear();
            }
        }

        for(auto element : *graph->ases_by_rank)
            delete element;
        graph->ases_by_rank->clear();

        for(auto element : *graph->components)
            delete element;
        graph->components->clear();

        graph->component_translation->clear();
        graph->stubs_to_parents->clear();
        graph->non_stubs->clear();

        //Re calculate the components with these new relationships
        graph->process(querier);

        //Do it again
        propagation_drop_connections_helper(prefix_blocks, subnet_blocks);
    } else {
        std::cout << "Round #" << round << ": No more attacks" << std::endl;
    }
}

/*
 * This will seed the announcement at the two different locations: the origin and the attacker
 * The attacker is sending as if it were a provider to the origin AS
 * We also write down what prefix the attcker is targetting
 */
void EZExtrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp /* = 0 */) {
    uint32_t path_origin_asn = as_path->at(as_path->size() - 1);

    auto result = graph->origin_victim_to_attacker->find(path_origin_asn);

    //Test if this is an attack
    if(result == graph->origin_victim_to_attacker->end()) {
        //Just run as usual then
        BlockedExtrapolator::give_ann_to_as_path(as_path, prefix, timestamp);
        return;
    }

    uint32_t attacker_asn = result->second.first;
    uint32_t victim2_asn = result->second.second;

    if(graph->destination_victim_to_prefixes->find(victim2_asn) != graph->destination_victim_to_prefixes->end()) {
        //only one attacking announcement per attacker, this way the metric is the percent of successful and *unique* attacks
        BlockedExtrapolator::give_ann_to_as_path(as_path, prefix, timestamp);
        return;
    }

    graph->destination_victim_to_prefixes->insert(std::make_pair(victim2_asn, prefix));

    EZAS* victim1 = this->graph->ases->find(path_origin_asn)->second;
    EZAS* attacker = this->graph->ases->find(attacker_asn)->second;

    //Create the "real" and "fake" announcement. These two will "compete" in propagation, and we want to see which ends up in the all_anns for victim 2
    EZAnnouncement realAnnouncement = EZAnnouncement(path_origin_asn, prefix.addr, prefix.netmask, 300, path_origin_asn, timestamp, true, false);
    EZAnnouncement attackAnnouncement = EZAnnouncement(path_origin_asn, prefix.addr, prefix.netmask, 299, path_origin_asn, timestamp, true, true);

    //Check if already recieved?
    victim1->process_announcement(realAnnouncement, this->random);
    attacker->process_announcement(attackAnnouncement, this->random);
}

/*
 * This will find the neighbor to the attacker on the AS path.
 * The initial call should have the as be the victim
 */
uint32_t EZExtrapolator::getPathNeighborOfAttacker(EZAS* as, Prefix<> &prefix, uint32_t attacker_asn) {
    uint32_t from_asn = as->all_anns->find(prefix)->second.received_from_asn;

    if(from_asn == attacker_asn)
        return as->asn;

    return getPathNeighborOfAttacker(graph->ases->find(from_asn)->second, prefix, attacker_asn);
}

/*
 * This runs at the end of every iteration
 * 
 * Baically, go to the victim and see if it chose the attacker route.
 * if the prefix didn't reach the victim (an odd edge case), then it does not count to the total
 * If the Victim chose the fake announcement path, then sucessful attack++
 * 
 * In addition, if there was a successful attack, record the edge (asn pair) from the attacker to the neighbor on the path
 */
void EZExtrapolator::calculate_successful_attacks() {
    for(auto& it : *graph->destination_victim_to_prefixes) {
        uint32_t victim_asn = it.first;
        EZAS* victim = graph->ases->find(victim_asn)->second;

        auto result = victim->all_anns->find(it.second);

        if(result == victim->all_anns->end())
            continue;

        if(result->second.from_attacker) {
            uint32_t attacker_asn = graph->origin_victim_to_attacker->find(result->second.origin)->second.first;
            uint32_t other_asn = getPathNeighborOfAttacker(victim, it.second, attacker_asn);

            graph->attacker_edges_removal->push_back(std::make_pair(attacker_asn, other_asn));

            successful_attacks++;
        }

        total_attacks++;
    }

    graph->destination_victim_to_prefixes->clear();
}

/*
 * A quick overwrite that removes the saving functionality since we are 
 *  only interested in the probibilities, not so much the actual info the extrapolator dumps out
 * Comment out the first line if the output is needed 
 */
void EZExtrapolator::save_results(int iteration) {
    // BaseExtrapolator::save_results(iteration);
    calculate_successful_attacks();
}