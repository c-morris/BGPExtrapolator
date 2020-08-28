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
                                uint32_t iteration_size,
                                uint32_t num_rounds,
                                uint32_t num_between) : BlockedExtrapolator(random_tiebraking, store_invert_results, store_depref_results, iteration_size) {
    
    graph = new EZASGraph();
    querier = new EZSQLQuerier(announcement_table, results_table, inverse_results_table, depref_results_table);
    
    this->successful_attacks = 0;
    this->successful_connections = 0;
    this->disconnections = 0;

    this->num_rounds = num_rounds;
    this->num_between = num_between;
}

EZExtrapolator::EZExtrapolator() 
    : EZExtrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                        ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, DEFAULT_ITERATION_SIZE, 
                        0, DEFAULT_NUM_ASES_BETWEEN_ATTACKER) { }

EZExtrapolator::~EZExtrapolator() { }

void EZExtrapolator::init() {
    BlockedExtrapolator::init();

    successful_attacks = 0;
    successful_connections = 0;
    disconnections = 0;
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
    
    std::ofstream ezStatistics("EZStatistics.csv");
    ezStatistics << "Round,Successful Attacks,Successful Connections,Disconnections,Total Attacks,Probability Successful Attack" << std::endl;

    uint32_t round = 0;

    //Propagate the graph, but after each round disconnect the attacker from the neighbor on the path
    //Runs until no more attacks (guaranteed to terminate since the edge to the neighebor is removed after an attack)
    do {
        round++;

        successful_attacks = 0;
        successful_connections = 0;
        disconnections = 0;

        std::cout << "Round #" << round << std::endl;

        extrapolate(prefix_blocks, subnet_blocks);

        if(successful_attacks > 0) {
            ezStatistics << round << "," << successful_attacks << "," << successful_connections << "," << disconnections << "," << graph->origin_to_attacker_victim->size() << "," << ((double) successful_attacks / (double) graph->origin_to_attacker_victim->size()) << std::endl;

            //Disconnect attacker from provider
            //Reset memory for the graph so it can calculate ranks, components, etc... accordingly
            if(num_between == 0)
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
        } else {
            std::cout << "Round #" << round << ": No more attacks" << std::endl;
        }
    } while(successful_attacks > 0 && round <= num_rounds - 1);
    
    // Cleanup
    delete prefix_blocks;
    delete subnet_blocks;

    ezStatistics.close();
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
    BlockedExtrapolator::give_ann_to_as_path(as_path, prefix, timestamp);
    
    uint32_t path_origin_asn = as_path->at(as_path->size() - 1);

    auto result = graph->origin_to_attacker_victim->find(path_origin_asn);

    //Test if this origin is the origin in an attack, if not, don't attack
    if(result == graph->origin_to_attacker_victim->end()) {
        // BlockedExtrapolator::give_ann_to_as_path(as_path, prefix, timestamp);
        return;
    }

    uint32_t attacker_asn = result->second.first;
    uint32_t victim2_asn = result->second.second;

    //Check if we have a prefix set to attack already, don't announce attack
    if(graph->victim_to_prefixes->find(victim2_asn) != graph->victim_to_prefixes->end())
        return;

    auto attacker_search = graph->ases->find(attacker_asn);
    if(attacker_search == graph->ases->end())
        return;
    
    EZAS* attacker = attacker_search->second;

    //Don't bother with anything of this if the attacker can't show up to the party
    // if(attacker->providers->size() == 0 && attacker->peers->size() == 0)
    //     return;

    graph->victim_to_prefixes->insert(std::make_pair(victim2_asn, prefix));

    EZAnnouncement attackAnnouncement = EZAnnouncement(path_origin_asn, prefix.addr, prefix.netmask, 299 - num_between, path_origin_asn, timestamp, true, true);
    attacker->process_announcement(attackAnnouncement, this->random_tiebraking);
}

uint32_t EZExtrapolator::getPathNeighborOfAttacker(EZAS* as, Prefix<> &prefix, uint32_t attacker_asn) {
    uint32_t from_asn = as->all_anns->find(prefix)->second.received_from_asn;

    if(from_asn == attacker_asn)
        return as->asn;

    return getPathNeighborOfAttacker(graph->ases->find(from_asn)->second, prefix, attacker_asn);
}

void EZExtrapolator::calculate_successful_attacks() {
    //For every victim, prefix pair
    for(auto& it : *graph->victim_to_prefixes) {
        uint32_t victim_asn = it.first;
        auto victim_search = graph->ases->find(victim_asn);

        if(victim_search == graph->ases->end())
            continue;

        EZAS* victim = victim_search->second;

        auto announcement_search = victim->all_anns->find(it.second);

        //If there is no announcement for the prefix, move on
        if(announcement_search == victim->all_anns->end()) { //the prefix never reached the victim
            disconnections++;
            continue;
        }

        //check if from attacker, then write down the edge between the attacker and neighbor on the path (through traceback)
        if(announcement_search->second.from_attacker) {
            if(this->num_between == 0) {
                uint32_t attacker_asn = graph->origin_to_attacker_victim->find(announcement_search->second.origin)->second.first;
                uint32_t neighbor_asn = getPathNeighborOfAttacker(victim, it.second, attacker_asn);
                graph->attacker_edge_removal->push_back(std::make_pair(attacker_asn, neighbor_asn));
            }

            successful_attacks++;
        } else {
            successful_connections++;
        }
    }

    graph->victim_to_prefixes->clear();
}

void EZExtrapolator::save_results(int iteration) {
    // BaseExtrapolator::save_results(iteration);
    calculate_successful_attacks();
}