#include "Graphs/EZASGraph.h"

EZASGraph::EZASGraph() : BaseGraph<EZAS>() {
    origin_victim_to_attacker = new std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>();
    destination_victim_to_prefixes = new std::unordered_map<uint32_t, std::vector<Prefix<>>*>();
}

EZASGraph::~EZASGraph() {
    delete origin_victim_to_attacker;
    delete destination_victim_to_prefixes;
}

EZAS* EZASGraph::createNew(uint32_t asn) {
    return new EZAS(asn);
}

void EZASGraph::distributeAttackersVictims(double percentage) {
    origin_victim_to_attacker->clear();
    destination_victim_to_prefixes->clear();

    std::vector<uint32_t> edge_ases_to_shuffle;
    for (auto &as : *ases) {
        //Only edge ASes (no customers) that have a provider 
        if(as.second->providers->size() >= 1 && as.second->customers->size() == 0) {
            edge_ases_to_shuffle.push_back(as.second->asn);    
        } 
    }

    std::size_t amount_to_pick = edge_ases_to_shuffle.size() * percentage;
    std::random_shuffle(edge_ases_to_shuffle.begin(), edge_ases_to_shuffle.end());

    for(int i = 0; i < amount_to_pick - (amount_to_pick % 3); i += 3) {
        uint32_t victim2 = edge_ases_to_shuffle.at(i + 2);

        origin_victim_to_attacker->insert(std::pair<uint32_t, std::pair<uint32_t, uint32_t>>
            (edge_ases_to_shuffle.at(i), std::make_pair
            (edge_ases_to_shuffle.at(i + 1), victim2)));
        
        destination_victim_to_prefixes->insert(std::make_pair(victim2, new std::vector<Prefix<>>));
    }
}

void EZASGraph::process(EZSQLQuerier* querier) {
    //We definately want stubs/edge ASes
    distributeAttackersVictims(0.25);
    tarjan();
    combine_components();
    save_supernodes_to_db(querier);
    decide_ranks();
}