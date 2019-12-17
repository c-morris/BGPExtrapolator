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
#include "ROVppAnnouncement.h"

//////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////

// These are the ROVppAS type flags
// They can be used to identify the type of ROVppAS
// and the in the set_rovpp_as_type function to set the type.
#define ROVPPAS_TYPE_BGP 0;        // Regular BGP
#define ROVPPAS_TYPE_ROV 1;        // Just standard ROV
#define ROVPPAS_TYPE_ROVPP 2;      // ROVpp 0.1 (Just Blackholing)
#define ROVPPAS_TYPE_ROVPPB 3;     // ROVpp 0.2 (Blackhole Announcements)
#define ROVPPAS_TYPE_ROVPPBP 4;    // ROVpp 0.3 (Preventive Ann with Blackhole Ann)

struct ROVppAS : public AS {
    std::vector<uint32_t> policy_vector;
    std::map<Prefix<>, Announcement> dropped_ann_map;  // Save dropped ann

    ROVppAS(uint32_t myasn=0,
        std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results=NULL,
        std::set<uint32_t> *prov=NULL,
        std::set<uint32_t> *peer=NULL,
        std::set<uint32_t> *cust=NULL);
    ~ROVppAS();

    void add_policy(uint32_t);
    // Overrided AS Methods
    // TODO: Uncomment once implemented, otherwise it causes tests to fail
    // void receive_announcement(Announcement &ann);
    // void receive_announcements(std::vector<Announcement> &announcements);

    // ROV Methods
    bool pass_rov(Announcement &ann);
};

#endif