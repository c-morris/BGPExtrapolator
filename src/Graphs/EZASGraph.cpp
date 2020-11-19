#include "Graphs/EZASGraph.h"

EZASGraph::EZASGraph() : BaseGraph(false, false) {
    origin_to_attacker_victim = new std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>();
    victim_to_prefixes = new std::unordered_map<uint32_t, Prefix<>>();
    // attacker_edge_removal = new std::vector<std::pair<uint32_t, uint32_t>>();

    adopters = new std::vector<uint32_t>();
}

EZASGraph::~EZASGraph() {
    delete origin_to_attacker_victim;
    delete victim_to_prefixes;
    // delete attacker_edge_removal;

    delete adopters;
}

EZAS* EZASGraph::createNew(uint32_t asn) {
    return new EZAS(asn);
}

// void EZASGraph::disconnectAttackerEdges() {
//     for(auto pair : *attacker_edge_removal) {
//         EZAS* attacker = ases->find(pair.first)->second;
//         EZAS* provider = ases->find(pair.second)->second;

//         auto result = provider->customers->find(attacker->asn);

//         if(result != provider->customers->end()) {//most likely
//             provider->customers->erase(result);
//             attacker->providers->erase(provider->asn);
//         } else {//super uncommon
//             provider->peers->erase(attacker->asn);
//             attacker->peers->erase(provider->asn);
//         }
//     }

//     attacker_edge_removal->clear();
// }

void EZASGraph::disconnect_as_from_adopting_neighbors(uint32_t asn) {
    auto as_search = ases->find(asn);
    if(as_search == ases->end())
        return;

    EZAS* as = as_search->second;
    for(uint32_t provider_asn : *as->providers) {
        auto provider_search = ases->find(provider_asn);
        if(provider_search == ases->end())
            continue;
            
        EZAS* provider = provider_search->second;

        if(!provider->adopter)
            continue;
        
        provider->customers->erase(asn);
        as->providers->erase(provider_asn);
    }

    for(uint32_t peer_asn : *as->peers) {
        auto peer_search = ases->find(peer_asn);
        if(peer_search == ases->end())
            continue;

        EZAS* peer = peer_search->second;

        if(!peer->adopter)
            continue;
        
        peer->peers->erase(asn);
        as->peers->erase(peer_asn);
    }

    for(uint32_t customer_asn : *as->customers) {
        auto customer_search = ases->find(customer_asn);
        if(customer_search == ases->end())
            continue;

        EZAS* customer = customer_search->second;

        if(!customer->adopter)
            continue;
        
        customer->providers->erase(asn);
        as->customers->erase(customer_asn);
    }
}

void EZASGraph::distributeAttackersVictims(SQLQuerier* querier) {
    origin_to_attacker_victim->clear();
    victim_to_prefixes->clear();

    pqxx::result triples = querier->execute("select * from good_customer_pairs");

    uint32_t origin_asn;
    uint32_t attacker_asn; 
    uint32_t victim_asn;

    for(unsigned int i = 0; i < triples.size(); i++) {
        triples[i]["victim_1"].to(origin_asn);
        triples[i]["attacker"].to(attacker_asn);
        triples[i]["victim_2"].to(victim_asn);

        auto origin_search = ases->find(origin_asn);
        auto attacker_search = ases->find(attacker_asn);
        auto victim_search = ases->find(victim_asn);

        //Only add if all three ASes exist
        //In addition, this ensures that none of them are in a component
        if(origin_search == ases->end() || attacker_search == ases->end() || victim_search == ases->end())
            continue;

        attacker_search->second->attacker = true;

        origin_to_attacker_victim->insert(std::pair<uint32_t, std::pair<uint32_t, uint32_t>>
            (origin_asn, std::make_pair(attacker_asn, victim_asn)));
    }

    //Distribute adopters
    for(auto ases_pair : *ases) {
        ases_pair.second->adopter = true;
    }
}

void EZASGraph::process(SQLQuerier* querier) {
    //We definately want stubs/edge ASes
    // distributeAttackersVictims(querier);
    tarjan();
    combine_components();
    //Don't need to save super nodes
    decide_ranks();
}