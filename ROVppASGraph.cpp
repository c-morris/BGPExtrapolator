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

#include "ROVppASGraph.h"

ROVppASGraph::ROVppASGraph() : ASGraph() {
    attackers = new std::set<uint32_t>();
    victims = new std::set<uint32_t>();
}

ROVppASGraph::~ROVppASGraph() { 
    delete attackers;
    delete victims;
}

/** Process the graph without removing stubs (needs querier to save them).
 */
void ROVppASGraph::process(SQLQuerier *querier) {
    // Main difference is remove_stubs isn't being called
    tarjan();
    combine_components();
    save_supernodes_to_db(querier);
    decide_ranks();
    return;
}

/** Generates an ASGraph from relationship data in an SQL database based upon:
 *      1) A populated peers table
 *      2) A populated customer_providers table
 *      3) A populated rovpp_ases table
 * 
 *
 *
 * @param querier
 */
void ROVppASGraph::create_graph_from_db(ROVppSQLQuerier *querier){
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

    // Assign policies to ASes
    for (auto policy_table : querier->policy_tables) {
        R = querier->select_AS_flags(policy_table);
        // For each AS in the policy Table
        for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
            // Get the ASN for current AS
            auto search = ases->find(c["asn"].as<uint32_t>());
            if (search != ases->end()) {
                // Add the policy to AS
                // TODO Handle "as_type" policy as an array
                dynamic_cast<ROVppAS*>(search->second)->add_policy(c["as_type"].as<uint32_t>());
            }
        }
    }
    process(querier);
    return;
}

/** Adds an ROVppAS relationship to the graph.
 *
 * If the ROVppAS does not exist in the graph, it will be created.
 *
 * @param asn ASN of AS to add the relationship to
 * @param neighbor_asn ASN of neighbor.
 * @param relation AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
 */
void ROVppASGraph::add_relationship(uint32_t asn,
                                    uint32_t neighbor_asn,
                                    int relation) {
    auto search = ases->find(asn);
    if (search == ases->end()) {
        // if AS not yet in graph, create it
        ases->insert(std::pair<uint32_t, AS*>(asn, new ROVppAS(asn, attackers)));
        search = ases->find(asn);
    }
    search->second->add_neighbor(neighbor_asn, relation);
}

void ROVppASGraph::to_graphviz(std::ostream &os, std::vector<uint32_t> asns) {
    os << "import graphviz" << std::endl;
    os << "dot = graphviz.Digraph()" << std::endl;
    os << "dot.attr('edge',arrowsize='0.5')" << std::endl;
    for (auto asn : asns) {
        ROVppAS &as = *dynamic_cast<ROVppAS*>(ases->find(asn)->second);
        // create node
        os << "dot.node('" << as.asn << "', '" << as.asn << "'";
        // green if implementing
        os << ((as.policy_vector.size() > 0 && as.policy_vector.at(0) != 0) ? ", color='green'" : " ");
        os << ")" << std::endl;
        // neighbors
        for (uint32_t other_asn : *as.customers) {
            ROVppAS &other_as = *dynamic_cast<ROVppAS*>(ases->find(other_asn)->second);
            os << "dot.node('" << other_as.asn << "', '" << other_as.asn << "'";
            // green if implementing
            os << ((other_as.policy_vector.size() > 0 && other_as.policy_vector.at(0) != 0) ? ", color='green'" : " ");
            os << ")" << std::endl;
        }

        for (uint32_t other_asn : *as.peers) {
            ROVppAS &other_as = *dynamic_cast<ROVppAS*>(ases->find(other_asn)->second);
            os << "dot.node('" << other_as.asn << "', '" << other_as.asn << "'";
            os << ((other_as.policy_vector.size() > 0 && other_as.policy_vector.at(0) != 0) ? ", color='green'" : " ");
            os << ")" << std::endl;
        }
        for (uint32_t other_asn : *as.providers) {
            ROVppAS &other_as = *dynamic_cast<ROVppAS*>(ases->find(other_asn)->second);
            os << "dot.node('" << other_as.asn << "', '" << other_as.asn << "'";
            os << ((other_as.policy_vector.size() > 0 && other_as.policy_vector.at(0) != 0) ? ", color='green'" : " ");
            os << ")" << std::endl;
        }
        // relationships
        for (auto other : *as.customers) {
            os << "dot.edge('" << as.asn << "', '" << other << "')" << std::endl;
        }
        for (auto other : *as.peers) {
            os << "dot.edge('" << as.asn << "', '" << other << "', dir='none')" << std::endl;
        }
        for (auto other : *as.providers) {
            os << "dot.edge('" << other << "', '" << as.asn << "')" << std::endl;
        }
        // announcements
        for (auto ann : *as.all_anns) {
            os << "dot.edge('" << ann.second.received_from_asn << "', '" << as.asn << "', " << 
            (as.pass_rov(ann.second) ? "color='blue'" : "color='red'")
            << ", label='" << ann.second.prefix.to_cidr() << "')" << std::endl;
            if (ann.second.received_from_asn != asn && ann.second.received_from_asn != UNUSED_ASN_FLAG_NOTHIJACKED && ann.second.received_from_asn != 64513 && ann.second.received_from_asn != 64512) {
                to_graphviz_traceback(os, ann.second.received_from_asn, 0);
            }
        }
    }
    os << "dot.render('test-output/rovppextrapolator2.gv')";
}

void ROVppASGraph::to_graphviz_traceback(std::ostream &os, uint32_t asn, int depth) {
    auto search = ases->find(asn);
    if (search == ases->end()) { return; }
    ROVppAS *tmp_ptr = dynamic_cast<ROVppAS*>(search->second);
    if (tmp_ptr == NULL) { return; }
    ROVppAS &as = *tmp_ptr;
    // create node
    os << "dot.node('" << as.asn << "', '" << as.asn << "'";
    // green if implementing
    os << ((as.policy_vector.size() > 0 && as.policy_vector.at(0) != 0) ? ", color='green'" : " ");
    os << ")" << std::endl;
    
    for (auto ann : *as.all_anns) {
        os << "dot.edge('" << ann.second.received_from_asn << "', '" << as.asn << "', " << 
        (ann.second.origin == 64512 ? "color='grey'" :  (as.pass_rov(ann.second) ? "color='blue'" : "color='red'"))
        << ", label='" << ann.second.prefix.to_cidr() << "')" << std::endl;
        if (ann.second.received_from_asn != asn && ann.second.received_from_asn != UNUSED_ASN_FLAG_NOTHIJACKED && ann.second.received_from_asn != 64513 && ann.second.received_from_asn != 64512 && depth < 3) {
            if (as.customers->find(ann.second.received_from_asn) != as.customers->end()) {
                os << "dot.edge('" << as.asn << "', '" << ann.second.received_from_asn << "')" << std::endl;
            }
            if (as.peers->find(ann.second.received_from_asn) != as.peers->end()) {
                os << "dot.edge('" << as.asn << "', '" << ann.second.received_from_asn << "', dir='none')" << std::endl;
            }
            if (as.providers->find(ann.second.received_from_asn) != as.providers->end()) {
                os << "dot.edge('" << ann.second.received_from_asn << "', '" << as.asn << "')" << std::endl;
            }
            to_graphviz_traceback(os, ann.second.received_from_asn, depth+1);
        }
    }
}
