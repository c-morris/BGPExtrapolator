#ifndef EZ_AS_GRAPH_H
#define EZ_AS_GRAPH_H

#include "ASes/EZAS.h"
#include "Graphs/BaseGraph.h"
#include "SQLQueriers/EZSQLQuerier.h"

class EZASGraph : public BaseGraph<EZAS> {
public:
    //Victim 1: attacker, victim2
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> *origin_victim_to_attacker;

    //Victim 2, prefixes to check
    std::unordered_map<uint32_t, std::vector<Prefix<>>*> *destination_victim_to_prefixes;

    EZASGraph();
    ~EZASGraph();

    EZAS* createNew(uint32_t asn);

    void distributeAttackersVictims(double percentage);
    void process(SQLQuerier* querier);
};

#endif