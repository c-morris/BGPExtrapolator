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

#ifndef ASGRAPH_H
#define ASGRAPH_H

// Define relationship macros
#define AS_REL_PROVIDER 0
#define AS_REL_PEER 100
#define AS_REL_CUSTOMER 200

class SQLQuerier;

#include <map>
#include <unordered_map>
#include <vector>
#include <stack>
#include <sys/stat.h>
#include <dirent.h>
#include <pqxx/pqxx>

#include "AS.h"
#include "SQLQuerier.h"
#include "TableNames.h"

class ASGraph {
public:
    std::map<uint32_t, AS*> *ases;            // Map of ASN to AS object
    std::vector<std::set<uint32_t>*> *ases_by_rank;     // Vector of ranks
    std::vector<std::vector<uint32_t>*> *components;    // Strongly connected components
    std::map<uint32_t, uint32_t> *component_translation;// Translate AS to supernode AS
    std::map<uint32_t, uint32_t> *stubs_to_parents;
    std::vector<uint32_t> *non_stubs;
    std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results;

    ASGraph();
    virtual ~ASGraph();
    // Propagation interaction
    void clear_announcements();
    uint32_t translate_asn(uint32_t asn);
    // Graph setup
    virtual void add_relationship(uint32_t asn, uint32_t neighbor_asn, int relation);
    void process(); // Remove?
    virtual void process(SQLQuerier *querier);
    virtual void create_graph_from_db(SQLQuerier *querier);
    void remove_stubs(SQLQuerier *querier);
    void save_stubs_to_db(SQLQuerier *querier);
    void save_non_stubs_to_db(SQLQuerier *querier);
    void save_supernodes_to_db(SQLQuerier *querier);
    void decide_ranks();
    // Supernode generation
    void tarjan();
    void tarjan_helper(AS *as, int &index, std::stack<AS*> &s);
    void combine_components();
    // Misc
    void printDebug();
    void to_graphviz(std::ostream &os);
    friend std::ostream& operator<<(std::ostream &os, const ASGraph& asg);    
};
#endif
