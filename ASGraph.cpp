#include <cstdint>
#include <iostream>
#include <set>
#include <vector>
#include <stack>
#include <algorithm>
#include "ASGraph.h"
#include "AS.h"

ASGraph::ASGraph() {
    ases = new std::map<uint32_t, AS*>;    
    ases_by_rank = new std::vector<std::set<uint32_t>*>(255);
}

ASGraph::~ASGraph() {
    for (auto const& as : *ases) {
        delete as.second;
    }
    delete ases;
    delete ases_by_rank;
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
 */
std::vector<std::vector<uint32_t>*>* ASGraph::decide_ranks() {
    // initialize the sets
    for (int i = 0; i < 255; i++) {
        (*ases_by_rank)[i] = new std::set<uint32_t>();
    }
    // initial set of customer ASes at the bottom of the DAG
    for (auto &as : *ases) {
        if (as.second->customers->empty()) {
            // if AS is a leaf node
            (*ases_by_rank)[0]->insert(as.first);
            as.second->rank = 0;
        }
    }
    
    // this is decreased from 1000 to 254 because the TTL on an IPv4 packet
    // is at most 255, so in theory this is a reasonable maximum for ASes too.
    for (int i = 0; i < 254; i++) {
        if ((*ases_by_rank)[i]->empty()) {
            // if we are above the top of the DAG, stop
            return NULL;
        }
        for (uint32_t asn : *(*ases_by_rank)[i]) {
            for (const uint32_t &provider_asn : *ases->find(asn)->second->providers) {
                auto prov_AS = ases->find(provider_asn)->second;
                if (prov_AS->rank == -1) {
                    int skip_provider = 0;
                    // TODO improve variable naming here
                    for (auto prov_cust_asn : *prov_AS->customers) {
                        auto *prov_cust_AS = ases->find(prov_cust_asn)->second;
                        if (prov_cust_AS->rank == -1 ||
                            // TODO also check SCC stuff
                            prov_cust_AS->rank > i) {
                            continue;
                        }
                        ases->find(provider_asn)->second->rank = i + 1;
                        (*ases_by_rank)[i+1]->insert(provider_asn);
                    }
                }
            }
        }
    }

    // do not free this memory, not done with it yet
    //// should be less than 255 iterations
    //for (size_t i = 0; i < ases_by_rank->size(); i++) {
    //    delete (*ases_by_rank)[i];
    //}
}


//https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
std::vector<std::vector<uint32_t>*>* ASGraph::tarjan() {
    int index = 0;
    std::stack<AS*> s;
    std::vector<std::vector<uint32_t>*>* components = new std::vector<std::vector<uint32_t>*>;

    for (auto &as : *ases) {
        if (as.second->index == -1){
            tarjan_helper(as.second, index, s, components);
        }
    }
    for (auto &component : *components){
        delete component;
    }
    delete components;

    return components;
}

//TODO replace 4 long args with a struct
void ASGraph::tarjan_helper(AS *as, int &index, std::stack<AS*> &s, 
                            std::vector<std::vector<uint32_t>*>* components) {
    as->index = index;
    as->lowlink = index;
    index++;
    s.push(as);
    as->onStack = true;

    for (auto &neighbor : *(as->providers)){
        AS *n = ases->find(neighbor)->second;
        if (n->index == -1){
            tarjan_helper(n, index, s,components);
            as->lowlink = std::min(as->lowlink,n->lowlink);
        }
        else if (n->onStack){
            as->lowlink = std::min(as->lowlink, n->index);
        }
    }
    if (as->lowlink == as->index){
        std::vector<uint32_t> *component = new std::vector<uint32_t>;
        AS *as_from_stack;
        do{
            as_from_stack = s.top();
            s.pop();
            as_from_stack->onStack = false;
            component->push_back(as_from_stack->asn);
        } while (as_from_stack != as);
        components->push_back(component);
    }
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


