#ifndef ASGRAPH_H
#define ASGRAPH_H

#include <map>
#include <vector>
#include <stack>
#include "AS.h"


struct ASGraph {
    std::map<uint32_t, AS*> *ases; // map of ASN to AS object 
    std::vector<uint32_t> *ases_with_anns;
    std::vector<std::set<uint32_t>*> *ases_by_rank;
    std::vector<std::vector<uint32_t>*> *components;
    //component_translation keeps key-value pairs for each ASN and the ASN
    //it became a part of due to strongly connected component combination.
    //This is used in Extrapolator.give_ann_to_as_path() where ASNs on an 
    //announcements AS_PATH need to be located.
    std::map<uint32_t, uint32_t> *component_translation;

    ASGraph();
    ~ASGraph();
    void add_relationship(uint32_t asn, uint32_t neighbor_asn, int relation);
    void decide_ranks();
    void process();
    void tarjan();
    void tarjan_helper(AS *as, int &index, std::stack<AS*> &s);
    void printDebug();
    void combine_components();
    friend std::ostream& operator<<(std::ostream &os, const ASGraph& asg);
};

#endif

