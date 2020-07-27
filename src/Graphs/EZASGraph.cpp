#include "Graphs/EZASGraph.h"

EZASGraph::EZASGraph() : BaseGraph<EZAS>() {
    origin_to_attacker_victim = new std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>();
    victim_to_prefixes = new std::unordered_map<uint32_t, Prefix<>>();
    attacker_edge_removal = new std::vector<std::pair<uint32_t, uint32_t>>();
}

EZASGraph::~EZASGraph() {
    delete origin_to_attacker_victim;
    delete victim_to_prefixes;
    delete attacker_edge_removal;
}

EZAS* EZASGraph::createNew(uint32_t asn) {
    return new EZAS(asn);
}

void EZASGraph::disconnectAttackerEdges() {
    for(auto pair : *attacker_edge_removal) {
        EZAS* attacker = ases->find(pair.first)->second;
        EZAS* provider = ases->find(pair.second)->second;

        auto result = provider->customers->find(attacker->asn);

        if(result != provider->customers->end()) {//most likely
            provider->customers->erase(result);
            attacker->providers->erase(provider->asn);
        } else {//super uncommon
            provider->peers->erase(attacker->asn);
            attacker->peers->erase(provider->asn);
        }
    }

    attacker_edge_removal->clear();
}

void EZASGraph::distributeAttackersVictims(SQLQuerier* querier) {
    origin_to_attacker_victim->clear();
    victim_to_prefixes->clear();

    pqxx::result triples = querier->execute("select * from good_customer_pairs");

    uint32_t victim1;
    uint32_t attacker; 
    uint32_t victim2;

    for(unsigned int i = 0; i < triples.size(); i++) {
        triples[i]["victim_1"].to(victim1);
        triples[i]["attacker"].to(attacker);
        triples[i]["victim_2"].to(victim2);

        origin_to_attacker_victim->insert(std::pair<uint32_t, std::pair<uint32_t, uint32_t>>
            (victim1, std::make_pair(attacker, victim2)));
    }
}

void EZASGraph::process(SQLQuerier* querier) {
    //We definately want stubs/edge ASes
    distributeAttackersVictims(querier);
    tarjan();
    combine_components();
    //Don't need to save super nodes
    decide_ranks();
}