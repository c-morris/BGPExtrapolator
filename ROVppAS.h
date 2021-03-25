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

// These are the ROVppAS type flags
// They can be used to identify the type of ROVppAS
// and the in the set_rovpp_as_type function to set the type.
#define ROVPPAS_TYPE_BGP 1        // Regular BGP (it's also 0)
#define ROVPPAS_TYPE_ROV 2        // Just standard ROV
#define ROVPPAS_TYPE_ROVPP0 3      // ROVpp 0 (Just don't send to bad neighbors)
#define ROVPPAS_TYPE_ROVPP 4      // ROVpp 1 (Just Blackholing)
#define ROVPPAS_TYPE_ROVPPB 6     // ROVpp 2a (Blackhole Announcements)
#define ROVPPAS_TYPE_ROVPPBP 7    // ROVpp 3 (Preventive Ann with Blackhole Ann)
#define ROVPPAS_TYPE_ROVPPBIS 5    // ROVpp 2 (Blackhole Ann to Customers Only)
#define ROVPPAS_TYPE_ROVPP_LITE 8      // ROVpp Lite 1 (Just Blackholing)
#define ROVPPAS_TYPE_ROVPPB_LITE 10     // ROVpp Lite 2a (Blackhole Announcements)
#define ROVPPAS_TYPE_ROVPPBP_LITE 9    // ROVpp Lite 3 (Preventive Ann with Blackhole Ann)
#define ROVPPAS_TYPE_ROVPPBIS_LITE 11    // ROVpp Lite 2 (Blackhole Ann to Customers Only)

// Special Constants 
// This is used for ROVpp 0.1+ to 
// identify blackhole announcements in the dataplane.
// We're also using this in contrl plane in pass_rovpp
// Constant was agreed upon with Simulation code.
#define UNUSED_ASN_FLAG_FOR_BLACKHOLES 64512  

struct ROVppAS : public AS {
    std::vector<uint32_t> policy_vector;
    std::set<uint32_t> *attackers;
    std::set<uint32_t> *bad_neighbors;  // neighbors that have sent us an attacker ann
    // Announcement Tracking Member Variables
    std::set<Announcement> *failed_rov;  // Save dropped announcements (i.e. attacker announcements)
    std::set<Announcement> *passed_rov;  // History of all announcements that have passed ROV
    std::set<Announcement> *blackholes;  // Keep track of blackholes created
    std::set<std::pair<Announcement,Announcement>> *preventive_anns;  // Keep track of preventive announcements and their alternatives

    ROVppAS(uint32_t myasn=0,
        std::set<uint32_t> *attackers=NULL,
        std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results=NULL,
        std::set<uint32_t> *prov=NULL,
        std::set<uint32_t> *peer=NULL,
        std::set<uint32_t> *cust=NULL);
    ~ROVppAS();
    
    // Overrided Methods
    void process_announcement(Announcement &ann, bool ran=true);
    void process_announcements(bool ran=true);
    
    // ROV Methods
    bool pass_rov(Announcement &ann);
    bool pass_rovpp(Announcement &ann);
    void add_policy(uint32_t);
    // Helper functions
    void withdraw(Announcement &ann);
    void withdraw(Announcement &ann, AS &neighbor);
    uint8_t tiny_hash(uint32_t);
    bool within_bad_neighbor_hole(Announcement &ann);
    void check_preventives(Announcement ann);
    Announcement best_alternative_route(Announcement &ann);  // help find a good alternative route 
                                                        // (i.e. an ann from a neighbor which 
                                                        // didn't give you the attacker's announcement)
                                                        // Will return the same given ann if there is
                                                        // no better alternative
    std::ostream& stream_blackholes(std:: ostream &os);
};

#endif

