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

#ifndef ROVPPAS_H
#define ROVPPAS_H

#include "AS.h"
#include "ROVppASGraph.h"
#include "ROVppAnnouncement.h"

struct ROVppAS : public AS {

    std::map<Prefix<>, Announcement> dropped_ann_map;  // Save dropped ann
    ROVppASGraph *rovpp_as_graph;
    bool rov_enabled = false;

    ROVppAS(uint32_t myasn=0,
        std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results=NULL,
        std::set<uint32_t> *prov=NULL,
        std::set<uint32_t> *peer=NULL,
        std::set<uint32_t> *cust=NULL);
    ~ROVppAS();

    // Overrided AS Methods
    // TODO: Uncomment once implemented, otherwise it causes tests to fail
    // void receive_announcement(Announcement &ann);
    // void receive_announcements(std::vector<Announcement> &announcements);

    // ROV Methods
    bool pass_rov(Announcement &ann);
    void enable_rov();
};

#endif
