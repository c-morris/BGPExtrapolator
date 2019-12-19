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

#include "ROVppAS.h"

ROVppAS::ROVppAS(uint32_t myasn,
       ROVppASGraph *as_graph,
       std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inv,
       std::set<uint32_t> *prov,
       std::set<uint32_t> *peer,
       std::set<uint32_t> *cust)
    : AS(myasn, inv, prov, peer, cust)  { 
    // Save pointer to graph
    rovpp_as_graph = as_graph;
}

ROVppAS::~ROVppAS() { }

/** Set the AS type
 *
 * An AS type maybe specified from the list of possible types listed in the header.
 * For example: ROVPPAS_TYPE_BGP, ROVPPAS_TYPE_ROV
 * 
 * 
 * @param type_flag The arguments to this flag are listed in the header file. For example ROVPPAS_TYPE_BGP
 */
void ROVppAS::set_rovpp_as_type(int type_flag) {
    rovpp_as_type = type_flag;
}

/**
 * Checks whether or not an announcement is from an attacker
 * 
 * @param  ann [description]
 * @return bool  true if from attacker, false otherwise
 */
bool ROVppAS::pass_rov(Announcement &ann) {
    auto result = rovpp_as_graph.attackers.find(ann.origin);
    if (result == rovpp_as_type.end()) {
        return false;
    } else {
        return true;
    }
}

/** Push the received announcements to the incoming_announcements vector.
 *
 * Note that this differs from the Python version in that it does not store
 * a dict of (prefix -> list of announcements for that prefix).
 *
 * @param announcements to be pushed onto the incoming_announcements vector.
 */
void ROVppAS::receive_announcements(std::vector<Announcement> &announcements) {
    for (Announcement &ann : announcements) {
        // Check if the Announcement is from attacker
        if (pass_rov(ann)) {
            // Do not check for duplicates here
            // push_back makes a copy of the announcement
            incoming_announcements->push_back(ann);
        }
    }
}
