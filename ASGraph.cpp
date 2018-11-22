#include <cstdint>
#include <iostream>
#include "ASGraph.h"
#include "AS.h"

ASGraph::ASGraph() {
    ases = new std::map<uint32_t, AS*>;    
}

ASGraph::~ASGraph() {
    //for (auto const& as : *ases) {
    //    delete as.second;
    //}
    delete ases;
}

void ASGraph::printDebug() {
    for (auto const& as : *ases) {
        std::cout << as.first << ':' << as.second->asn << std::endl;
    }
    return; 
}
