#include <cstdint>
#include <iostream>
#include "ASGraph.h"
#include "AS.h"

ASGraph::ASGraph() {
    ases = new std::map<uint32_t, AS*>;    
}

ASGraph::~ASGraph() {
    for (auto const& as : *ases) {
        delete as.second;
    }
    delete ases;
}

/** Adds an AS relationship to the graph.
 *
 * @param asn ASN of AS to add the relationship to
 * @param neighbor_asn ASN of neighbor.
 * @param relation AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
 */
void ASGraph::add_relationship(uint32_t asn, uint32_t neighbor_asn, 
    int relation) {
    auto search = ases->find(asn);
    if (search == ases->end()) {
        // if AS not yet in graph, create it
        ases->insert(std::pair<uint32_t, AS*>(asn, new AS(asn)));
        search = ases->find(asn);
    }
    search->second->add_neighbor(neighbor_asn, relation);
}

/** Decide and assign ranks to all the AS's in the graph. 
 *
 * Not sure why the list is returned since in the Python code it doesn't get
 * used, but since it's returned there I'll put it as the return type. 
 * 
 * @return I'm pretty sure this list never gets used, so for now, NULL
 */
std::vector<std::vector<uint32_t>*>* ASGraph::decide_ranks() {
    auto ases_by_rank = new std::vector<std::vector<uint32_t>*>;
    auto customer_ases = new std::vector<uint32_t>;
    for (auto &as : *ases) {
        if (as.second->customers->empty()) {
            customer_ases->push_back(as.first);
            as.second->rank = 0;
        }
    }
    ases_by_rank->push_back(customer_ases);
    
    for (int i = 0; i < 1000; i++) {
        auto ases_at_rank_i_plus_one = new std::vector<uint32_t>;
        // I don't understand why this part doesn't break the extrapolator, so
        // I'm not translating it until Michael explains it to me. 
        // if(i not in self.ases_by_rank):
        //     return self.ases_by_rank

        for (uint32_t asn : *(*ases_by_rank)[i]) {
            for (const uint32_t &provider_asn : *ases->find(asn)->second->providers) {
                auto prov_AS = ases->find(provider_asn)->second;
                //if (prov_AS->rank
            }
        }
    }

    // should be less than 1000 iterations
    for (size_t i = 0; i < ases_by_rank->size(); i++) {
        delete (*ases_by_rank)[i];
    }
    delete ases_by_rank;

    return NULL;
}

// print all as's
void ASGraph::printDebug() {
    for (auto const& as : *ases) {
        std::cout << as.first << ':' << as.second->asn << std::endl;
    }
    return; 
}

std::ostream& operator<<(std::ostream &os, const ASGraph& asg) {
    os << "AS's" << std::endl;
    for (auto const& as : *(asg.ases)) {
        os << *as.second << std::endl;
    }
    return os;
}


