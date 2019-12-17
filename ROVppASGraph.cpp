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

ROVppASGraph::ROVppASGraph() : ASGraph() {}

ROVppASGraph::~ROVppASGraph() { }

/** Generates an ASGraph from relationship data in an SQL database based upon:
 *      1) A populated peers table
 *      2) A populated customer_providers table
 *      3) A populated rovpp_ases table
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
    // Assemble policy arrays
    // TODO should take a parameter table name
    // TODO currently takes only a single policy
    R = querier->select_AS_flags();
    for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
        auto search = ases->find(c["asn"].as<uint32_t>());
        if (search != ases->end()) {
            //Polymorphic function
            search->second->add_policy(c["as_type"].as<uint32_t>());
        }
    }
    process(querier);
    return;
}

