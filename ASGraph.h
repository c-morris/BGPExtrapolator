#ifndef ASGRAPH_H
#define ASGRAPH_H

#include <map>
#include <vector>
#include "AS.h"


struct ASGraph {
    std::map<uint32_t, AS*> *ases; // map of ASN to AS object 
    std::vector<uint32_t> *ases_with_anns;
    std::vector<std::set<uint32_t>*> *ases_by_rank;


    ASGraph();
    ~ASGraph();
    void add_relationship(uint32_t asn, uint32_t neighbor_asn, int relation);
    std::vector<std::vector<uint32_t>*>* decide_ranks();
    void printDebug();
    friend std::ostream& operator<<(std::ostream &os, const ASGraph& asg);
};

#endif

