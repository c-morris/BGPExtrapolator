#include "ASGraph.h"
#include "AS.h"
#include <cstdint>
#include <iostream>

ASGraph::ASGraph() {
    ases = new std::map<uint32_t, AS*>;    
}

ASGraph::~ASGraph() {
    delete ases;
}

void ASGraph::printDebug() {
    for (auto const& as : *ases) {
        std::cout << as.first << ':' << as.second->asn << std::endl;
    }
    return; 
}
