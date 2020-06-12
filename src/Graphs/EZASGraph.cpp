#include "Graphs/EZASGraph.h"

EZASGraph::EZASGraph() : BaseGraph<EZAS>() {
    origin_victim_to_attacker = new std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>();
    destination_victim_to_prefixes = new std::unordered_map<uint32_t, Prefix<>>();
}

EZASGraph::~EZASGraph() {
    delete origin_victim_to_attacker;
    delete destination_victim_to_prefixes;
}

EZAS* EZASGraph::createNew(uint32_t asn) {
    return new EZAS(asn);
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