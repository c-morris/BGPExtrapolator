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

#ifndef ROVPPASGRAPH_H
#define ROVPPASGRAPH_H

#include "ASGraph.h"
#include "ROVppAS.h"
#include "ROVppSQLQuerier.h"

struct ROVppASGraph: public ASGraph {
    // Sets of ASNs to keep track of attackers and victims
    // TODO need set of vectors for policy index
    std::set<uint32_t> *attackers;
    std::set<uint32_t> *victims;
    
    ROVppASGraph();
    ~ROVppASGraph();

    // Overriden Methods
    void process(SQLQuerier *querier);
    void create_graph_from_db(ROVppSQLQuerier*);
    void add_relationship(uint32_t asn, uint32_t neighbor_asn, int relation);

    // Overloaded 
    void to_graphviz(std::ostream &os, std::vector<uint32_t> asns);
    void to_graphviz_traceback(std::ostream &os, uint32_t asn, int depth);
};
#endif
