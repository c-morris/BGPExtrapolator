/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#include <cstdint>
#include <iostream>
#include <set>
#include <vector>
#include <stack>
#include <algorithm>
#include <limits.h>

#include "Graphs/ASGraph.h"
#include "ASes/AS.h"

template <class ASType>
BaseGraph<ASType>::~BaseGraph() {
    for (auto const& as : *ases)
        delete as.second;
    delete ases;
    
    for (auto const& as : *ases_by_rank)
        delete as;
    delete ases_by_rank;

    for (auto const& c : *components)
        delete c;
    delete components;

    for (auto const& i : *inverse_results)
        delete i.second;
    delete inverse_results;

    delete component_translation;
    delete stubs_to_parents;
    delete non_stubs;
}

/** Clear all announcements in AS.
 */
template <class ASType>
void BaseGraph<ASType>::clear_announcements() {
    for (auto const& as : *ases)
        as.second->clear_announcements();
    for (auto const& i : *inverse_results)
        delete i.second;
    inverse_results->clear();
}

/** Adds an AS relationship to the graph.
 *
 * If the AS does not exist in the graph, it will be created.
 *
 * @param asn ASN of AS to add the relationship to
 * @param neighbor_asn ASN of neighbor.
 * @param relation AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
 */
template <class ASType>
void BaseGraph<ASType>::add_relationship(uint32_t asn, 
                                            uint32_t neighbor_asn, 
                                            int relation) {
    auto search = ases->find(asn);
    if (search == ases->end()) {
        // if AS not yet in graph, create it
        // ases->insert(std::pair<uint32_t, ASType*>(asn, new ASType(asn, inverse_results)));
        ases->insert(std::pair<uint32_t, ASType*>(asn, createNew(asn)));
        search = ases->find(asn);
    }
    search->second->add_neighbor(neighbor_asn, relation);
}

/** Translates asn to asn of component it belongs to in graph.
 *
 * @return 0 if asn isn't found, otherwise return identifying ASN
 */
template <class ASType>
uint32_t BaseGraph<ASType>::translate_asn(uint32_t asn) {
    auto search = component_translation->find(asn);
    if(search == component_translation->end())
        return asn; 
    return search->second;
}

/** Process with removing stubs (needs querier to save them).
 */
template <class ASType>
void BaseGraph<ASType>::process(SQLQuerier *querier) {
    remove_stubs(querier);
    tarjan();
    combine_components();
    save_supernodes_to_db(querier);
    decide_ranks();
}

/** Generates an ASGraph from relationship data in an SQL database based upon:
 *      1) A populated peers table
 *      2) A populated customer_providers table
 * 
 * @param querier
 */
template <class ASType>
void BaseGraph<ASType>::create_graph_from_db(SQLQuerier *querier) {
    // Assemble Peers
    pqxx::result R = querier->select_from_table(PEERS_TABLE);
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        add_relationship(c["peer_as_1"].as<uint32_t>(),
                         c["peer_as_2"].as<uint32_t>(),AS_REL_PEER);
        add_relationship(c["peer_as_2"].as<uint32_t>(),
                         c["peer_as_1"].as<uint32_t>(),AS_REL_PEER);
    }

    // Assemble Customer-Providers
    R = querier->select_from_table(CUSTOMER_PROVIDER_TABLE);
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        add_relationship(c["customer_as"].as<uint32_t>(),
                         c["provider_as"].as<uint32_t>(),AS_REL_PROVIDER);
        add_relationship(c["provider_as"].as<uint32_t>(),
                         c["customer_as"].as<uint32_t>(),AS_REL_CUSTOMER);
    }

    process(querier);
}

/** Remove the stub ASes from the graph.
 *
 * @param querier
 */
template <class ASType>
void BaseGraph<ASType>::remove_stubs(SQLQuerier *querier) {
    std::vector<ASType*> to_remove;
    // For all ASes in the graph
    for (auto &as : *ases) {
        // If this AS is a stub
        if(as.second->peers->size() == 0 &&
           as.second->providers->size() == 1 && 
           as.second->customers->size() == 0) {
            to_remove.push_back(as.second);    
        } else {
            non_stubs->push_back(as.first);
        }
    }
    // Handle stub removal
    for (auto *as : to_remove) {
        // Remove any edges to this stub from graph
        for(uint32_t provider_asn : *as->providers) {
            auto iter = ases->find(provider_asn);
            if (iter != ases->end()) {
                ASType* provider = iter->second;
                provider->customers->erase(as->asn);
            }
            stubs_to_parents->insert(std::pair<uint32_t, uint32_t>(as->asn,provider_asn));
        }
        
        // Remove from graph if it has not been already removed
        auto iter = ases->find(as->asn);
        if (iter != ases->end()) { 
            delete iter->second;
            ases->erase(iter);
        }
    }

    save_stubs_to_db(querier);
    save_non_stubs_to_db(querier);
}

/** Saves the stub ASes to be removed to a table on the database.
 *
 * @param querier
 */
template <class ASType>
void BaseGraph<ASType>::save_stubs_to_db(SQLQuerier *querier) {
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir)
        mkdir("/dev/shm/bgp",0777);
    else
        closedir(dir);

    std::ofstream outfile;
    std::cout << "Saving Stubs..." << std::endl;
    std::string file_name = "/dev/shm/bgp/stubs.csv";
    outfile.open(file_name);

    for (auto &stub : *stubs_to_parents)
        outfile << stub.first << "," << stub.second << "\n";
    
    outfile.close();
    querier->copy_stubs_to_db(file_name);
    std::remove(file_name.c_str());
}

/** Saves the non_stub ASes to a table on the database.
 *
 * @param querier
 */
template <class ASType>
void BaseGraph<ASType>::save_non_stubs_to_db(SQLQuerier *querier) {
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir)
        mkdir("/dev/shm/bgp",0777);
    else
        closedir(dir);

    std::ofstream outfile;
    std::cout << "Saving Non-Stubs..." << std::endl;
    std::string file_name = "/dev/shm/bgp/non-stubs.csv";
    outfile.open(file_name);

    for (auto non_stub : *non_stubs)
        outfile << non_stub << "\n";

    outfile.close();
    querier->copy_non_stubs_to_db(file_name);
    std::remove(file_name.c_str());
}

/** Generate a csv with all supernodes, then dump them to database.
 *
 * @param querier
 */
template <class ASType>
void BaseGraph<ASType>::save_supernodes_to_db(SQLQuerier *querier) {
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir)
        mkdir("/dev/shm/bgp",0777);
    else
        closedir(dir);

    std::ofstream outfile;
    std::cout << "Saving Supernodes..." << std::endl;
    std::string file_name = "/dev/shm/bgp/supernodes.csv";
    outfile.open(file_name); 
    
    // Iterate over each strongly connected components
    for (auto &cur_node : *components) {
        // For each supernode
        if (cur_node->size() > 1) {
            // Find the lowest ASN in supernode
            uint32_t low = UINT_MAX;
            for (auto &cur_asn : *cur_node) {
                if (cur_asn < low) 
                    low = cur_asn;
            }
            // Assemble rows as pairs; ASN in supernode, lowest ASN in that supernode
            for (auto &cur_asn : *cur_node)
                outfile << cur_asn << "," << low << "\n";
        }
    }

    outfile.close();
    querier->copy_supernodes_to_db(file_name);
    std::remove(file_name.c_str());
}

/** Decide and assign ranks to all the AS's in the graph. 
 *
 *  The rank of an AS one plus is the maximum level of the nodes it has below it. 
 *  This means it is possible to have an AS of rank 0 directly below an AS of 
 *  rank 4, but not possible to have an AS of rank 3 below one of rank 2. 
 *
 *  The bottom of the DAG is rank 0. 
 */
template <class ASType>
void BaseGraph<ASType>::decide_ranks() {
    ases_by_rank->clear();

    // Initial set of customer ASes at the bottom of the DAG
    ases_by_rank->push_back(new std::set<uint32_t>());
    // For ASes with no customers
    for (auto &as : *ases) {
        // If AS is a leaf node
        if (as.second->customers->empty()) {
            (*ases_by_rank)[0]->insert(as.first);
            as.second->rank = 0;
        }
    }
    
    int i = 0;
    // While there are elements to process current rank
    while (!(*ases_by_rank)[i]->empty()) {
        ases_by_rank->push_back(new std::set<uint32_t>());
        for (uint32_t asn : *(*ases_by_rank)[i]) {
            //For all providers of this AS
            for (const uint32_t &provider_asn : *ases->find(asn)->second->providers) {
                ASType* prov_AS = ases->find(translate_asn(provider_asn))->second;
                int oldrank = prov_AS->rank;
                // Move provider up to next rank
                if (oldrank < i + 1) {
                    prov_AS->rank = i + 1;
                    (*ases_by_rank)[i+1]->insert(provider_asn);
                    if (oldrank != -1) {
                        (*ases_by_rank)[oldrank]->erase(provider_asn);
                    }
                }
            }
        }
        i++;
    } 
    return;
}

/** Tarjan driver to detect strongly connected components in the ASGraph.
 * 
 * https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
 */
template <class ASType>
void BaseGraph<ASType>::tarjan() {
    int index = 0;
    std::stack<ASType*> s;

    for (auto &as : *ases)
        if (as.second->index == -1)
            tarjan_helper(as.second, index, s);
    return;
}

/** Tarjan algorithm to detect strongly connected components in the ASGraph.
 */
template <class ASType>
void BaseGraph<ASType>::tarjan_helper(ASType *as, int &index, std::stack<ASType*> &s) {
    as->index = index;
    as->lowlink = index;
    index++;
    s.push(as);
    as->onStack = true;
    
    for (auto &neighbor : *(as->providers)) {
        ASType *n = ases->find(neighbor)->second;
        if (n->index == -1) {
            tarjan_helper(n, index, s);
            as->lowlink = std::min(as->lowlink, n->lowlink);
        } else if (n->onStack) {
            as->lowlink = std::min(as->lowlink, n->index);
        }
    }

    if (as->lowlink == as->index) {
        std::vector<uint32_t> *component = new std::vector<uint32_t>;
        ASType *as_from_stack;
        do {
            as_from_stack = s.top();
            s.pop();
            as_from_stack->onStack = false;
            component->push_back(as_from_stack->asn);
        } while (as_from_stack != as);
        /** TODO Add logging here
        if (component->size() > 1)
            std::cout << "Supernode found of size " << component->size() << std::endl;
        */
        components->push_back(component);
    }
}

/** Combine providers, peers, and customers of ASes in a strongly connected component.
 *  Also append to component_translation for future reference.
 */
template <class ASType>
void BaseGraph<ASType>::combine_components() {
    
    // For each strongly connected component
    for (auto const& component : *components) {
        // Ignore single AS nodes
        if (component->size() <= 1)
            continue;
        
        // Find indentifying ASN for supernode 
        uint32_t combined_asn = component->at(0);
        for (auto &cur_asn : *component)
            if (cur_asn < combined_asn)
                combined_asn = cur_asn;

        // Combined Component will id as lowest ASN
        // AS *combined_AS = new AS(combined_asn, inverse_results);
        ASType *combined_AS = createNew(combined_asn);
        
        // For all members of a component, gather neighbors
        for (auto &cur_asn : *component) {
            combined_AS->member_ases->push_back(cur_asn);
            // Get the AS object associated to ASN
            auto asn_search = ases->find(cur_asn);
            ASType *cur_AS = asn_search->second;

            // Handle providers
            for (auto &provider_asn : *cur_AS->providers) {
                // Check if provider is in component
                bool external = (std::find(component->begin(), 
                                           component->end(), 
                                           provider_asn) == component->end());
                if (external) {
                    ASType *provider_AS = ases->find(provider_asn)->second;
                    // Add new relationship
                    combined_AS->add_neighbor(provider_asn, AS_REL_PROVIDER);
                    provider_AS->add_neighbor(combined_asn, AS_REL_CUSTOMER);
                    // Handle overlapping peer, remove peer relationship from supernode
                    combined_AS->remove_neighbor(provider_asn, AS_REL_PEER);
                    // Remove old subnode customer relationship from external provider
                    provider_AS->remove_neighbor(cur_asn, AS_REL_CUSTOMER);
                    // Remove subnode peer relationship from external provider if it exists
                    provider_AS->remove_neighbor(cur_asn, AS_REL_PEER);
                }
            }

            // Handle customers
            for (auto &customer_asn : *cur_AS->customers) {
                // Check if customer is in component
                bool external = (std::find(component->begin(), 
                                           component->end(), 
                                           customer_asn) == component->end());
                if (external) {
                    ASType *customer_AS = ases->find(customer_asn)->second;
                    // Add new relationship
                    combined_AS->add_neighbor(customer_asn, AS_REL_CUSTOMER);
                    customer_AS->add_neighbor(combined_asn, AS_REL_PROVIDER);
                    // Handle overlapping peer, remove redundant peer relationship from supernode
                    combined_AS->remove_neighbor(customer_asn, AS_REL_PEER);
                    // Remove old subnode provider relationship from external provider
                    customer_AS->remove_neighbor(cur_asn, AS_REL_PROVIDER);
                    // Remove redundant subnode peer relationship from external provider if it exists
                    customer_AS->remove_neighbor(cur_asn, AS_REL_PEER);
                }
            }   

            // Handle peers
            for (auto &peer_asn: *cur_AS->peers) {
                // Check if peer is in component
                bool external = (std::find(component->begin(), 
                                      component->end(), 
                                      peer_asn) == component->end());
                // Check if peer is already a provider in the combined AS
                bool no_provider_rel = (combined_AS->providers->find(peer_asn) ==
                                        combined_AS->providers->end());
                // Check if peer is already a customer in the combined AS
                bool no_customer_rel = (combined_AS->customers->find(peer_asn) ==
                                        combined_AS->customers->end());
                
                // Peer is safe to add to combined AS
                if (external && no_provider_rel && no_customer_rel) {
                    ASType *peer_AS = ases->find(peer_asn)->second;
                    // Add new relationship
                    combined_AS->add_neighbor(peer_asn, AS_REL_PEER);
                    peer_AS->add_neighbor(combined_asn, AS_REL_PEER);
                    // Remove old peer relation to the subnode
                    peer_AS->remove_neighbor(cur_asn, AS_REL_PEER);
                } else if (external) {
                    // Other relationship superseeds peer
                    ASType *peer_AS = ases->find(peer_asn)->second;
                    // Remove the subnode peer relationship from the external node
                    peer_AS->remove_neighbor(cur_asn, AS_REL_PEER);
                }
            }
            // Append ASN translation and discard member
            component_translation->insert(std::pair<uint32_t, uint32_t>(cur_asn,combined_asn));
            ases->erase(cur_AS->asn);
            delete cur_AS;
        }
        // Insert complete combined node to ases
        ases->insert(std::pair<uint32_t, ASType*>(combined_asn,combined_AS));
    }
    return;
}


/** Print all ASes for debug.
 */
template <class ASType>
void BaseGraph<ASType>::printDebug() {
    for (auto const& as : *ases)
        std::cout << as.first << ':' << as.second->asn << std::endl;
    return; 
}


/** Output python code for making graphviz digraph.
 */
template <class ASType>
void BaseGraph<ASType>::to_graphviz(std::ostream &os) {
    std::string id = "";
    for (auto const &as : *ases) {
        os << "dot.node('" << as.second->asn << "', '" << as.second->asn << "')" << std::endl;
        for (auto customer : *as.second->customers) {
            os << "dot.edge('" << as.second->asn << "', '" << customer << "')" << std::endl;
        }
    }
}


/** Operation for debug printing AS
 */
template <class U>
std::ostream& operator<<(std::ostream &os, const BaseGraph<U>& asg) {
    os << "AS's" << std::endl;
    for (auto const& as : *(asg.ases))
        os << *as.second << std::endl;
    return os;
}

template class BaseGraph<AS>;
template class BaseGraph<EZAS>;
template class BaseGraph<ROVppAS>;