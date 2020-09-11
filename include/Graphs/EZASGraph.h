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
    // std::vector<std::pair<uint32_t, uint32_t>> *attacker_edge_removal;

    //list of adopters in the graph
    std::vector<uint32_t> *adopters;

    EZASGraph();
    ~EZASGraph();

    EZAS* createNew(uint32_t asn);

    // void disconnectAttackerEdges();

    /**
     *  Given some asn (which has been determined to be malicious) and disconnect it from any adopting neighbors
     *  Make sure to re-process the graph after doing this because the components of the graph may not be the same after this removal
     *  Don't run this in the middle of the round either. It may invalidate the connected components of the graph
     * 
     *  @param asn -> The suspect AS that adopters should disconnect from
     */
    void disconnect_as_from_adopting_neighbors(uint32_t asn);
    void distributeAttackersVictims(SQLQuerier* querier);
    void process(SQLQuerier* querier);
};
#endif