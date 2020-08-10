#ifndef EZ_AS_GRAPH_H
#define EZ_AS_GRAPH_H

#include "ASes/EZAS.h"
#include "Graphs/BaseGraph.h"
#include "SQLQueriers/EZSQLQuerier.h"

class EZASGraph : public BaseGraph<EZAS> {
public:
    //Victim 1 (origin): attacker, victim2
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> *origin_to_attacker_victim;

    //Victim 2, prefixes to check
    std::unordered_map<uint32_t, Prefix<>> *victim_to_prefixes;

    //Attacker, Provider
    std::vector<std::pair<uint32_t, uint32_t>> *attacker_edge_removal;

    EZASGraph();
    ~EZASGraph();

    EZAS* createNew(uint32_t asn);

    void disconnectAttackerEdges();
    void distributeAttackersVictims(SQLQuerier* querier);
    void process(SQLQuerier* querier);
};
#endif