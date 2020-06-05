#include "Graphs/EZASGraph.h"

EZASGraph::EZASGraph() : BaseGraph<EZAS>() {
    origin_victim_to_attacker = new std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>();
    destination_victim_to_prefixes = new std::unordered_map<uint32_t, std::vector<Prefix<>>*>();
}

EZASGraph::~EZASGraph() {
    delete origin_victim_to_attacker;
    delete destination_victim_to_prefixes;
}

EZAS* EZASGraph::createNew(uint32_t asn) {
    return new EZAS(asn);
}