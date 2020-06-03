#ifndef EZ_AS_GRAPH_H
#define EZ_AS_GRAPH_H

#include "ASes/EZAS.h"
#include "Graphs/BaseGraph.h"

class EZASGraph : public BaseGraph<EZAS> {
public:
    std::set<uint32_t> *attackers;
    std::set<uint32_t> *victims;

    EZASGraph();
    ~EZASGraph();

    EZAS* createNew(uint32_t asn);
};

#endif