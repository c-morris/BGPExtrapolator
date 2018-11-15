#include <map>
#include "AS.h"

#ifndef ASGRAPH_H
#define ASGRAPH_H

struct ASGraph {
    std::map<uint32_t, AS*> *ases; // map of ASN to AS object 

    ASGraph();
    void printDebug();
};

#endif

