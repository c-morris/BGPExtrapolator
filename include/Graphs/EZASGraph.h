#ifndef EZ_AS_GRAPH_H
#define EZ_AS_GRAPH_H

#include "ASes/EZAS.h"
#include "Graphs/BaseGraph.h"

class EZASGraph : public BaseGraph<EZAS> {
public:
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> *origin_victim_to_attacker;
    std::unordered_map<uint32_t, std::vector<Prefix<>>*> *destination_victim_to_prefixes;

    EZASGraph();
    ~EZASGraph();

    EZAS* createNew(uint32_t asn);
};

#endif