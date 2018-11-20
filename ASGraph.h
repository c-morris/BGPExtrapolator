#ifndef ASGRAPH_H
#define ASGRAPH_H

#include <map>
#include <vector>
#include "AS.h"


struct ASGraph {
    std::map<uint32_t, AS*> *ases; // map of ASN to AS object 
    std::vector<uint32_t> *ases_with_anns;

    ASGraph();
    ~ASGraph();
    void printDebug();
};

#endif

