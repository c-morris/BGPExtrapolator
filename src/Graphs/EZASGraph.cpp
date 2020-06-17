#include "Graphs/EZASGraph.h"

EZASGraph::EZASGraph() : BaseGraph<EZAS>() {
    origin_victim_to_attacker = new std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>();
    destination_victim_to_prefixes = new std::unordered_map<uint32_t, Prefix<>>();
    attacker_edges_removal = new std::vector<std::pair<uint32_t, uint32_t>>();
}

EZASGraph::~EZASGraph() {
    delete origin_victim_to_attacker;
    delete destination_victim_to_prefixes;
}

EZAS* EZASGraph::createNew(uint32_t asn) {
    return new EZAS(asn);
}

void EZASGraph::disconnectAttackerEdges() {
    for(auto pair : *attacker_edges_removal) {
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

    attacker_edges_removal->clear();
}

void EZASGraph::distributeAttackersVictims(SQLQuerier* querier) {
    origin_victim_to_attacker->clear();
    destination_victim_to_prefixes->clear();

    pqxx::result triples = querier->execute("select * from good_customer_pairs");

    for(int i = 0; i < triples.size(); i++) {
        uint32_t victim1;
        uint32_t attacker; 
        uint32_t victim2;

        triples[i]["victim_1"].to(victim1);
        triples[i]["attacker"].to(attacker);
        triples[i]["victim_2"].to(victim2);

        origin_victim_to_attacker->insert(std::pair<uint32_t, std::pair<uint32_t, uint32_t>>
            (victim1, std::make_pair(attacker, victim2)));
    }
}

void EZASGraph::process(SQLQuerier* querier) {
    //We definately want stubs/edge ASes
    distributeAttackersVictims(querier);
    tarjan();
    combine_components();
    save_supernodes_to_db(querier);
    decide_ranks();
}