#include "Graphs/EZASGraph.h"

EZASGraph::EZASGraph() : BaseGraph<EZAS>() {
    attackers = new std::set<uint32_t>();
    victims = new std::set<uint32_t>();
}

EZASGraph::~EZASGraph() {
    delete attackers;
    delete victims;
}

EZAS* EZASGraph::createNew(uint32_t asn) {
    return new EZAS(asn, attackers);
}