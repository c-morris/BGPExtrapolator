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
#define ROVPPAS_TYPE_BGP 0        // Regular BGP
#define ROVPPAS_TYPE_ROV 1        // Just standard ROV
#define ROVPPAS_TYPE_ROVPP 2      // ROVpp 0.1 (Just Blackholing)
#define ROVPPAS_TYPE_ROVPPB 3     // ROVpp 0.2 (Blackhole Announcements)
#define ROVPPAS_TYPE_ROVPPBP 4    // ROVpp 0.3 (Preventive Ann with Blackhole Ann)

struct ROVppAS : public AS {
    std::vector<uint32_t> policy_vector;
    std::set<uint32_t> *attackers;
    // Announcement Tracking Member Variables
    std::vector<Announcement> *failed_rov; // Save dropped announcements (i.e. attacker announcements)
    std::vector<Announcement> *passed_rov; // History of all announcements that have passed ROV
    std::vector<Announcement> *blackholes;  // Keep track of blackholes created
    

    ROVppAS(uint32_t myasn=0,
        std::set<uint32_t> *attackers=NULL,
        std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results=NULL,
        std::set<uint32_t> *prov=NULL,
        std::set<uint32_t> *peer=NULL,
        std::set<uint32_t> *cust=NULL);
    ~ROVppAS();
    
    // Overrided Methods
    void process_announcements();
    
    
    // ROV Methods
    bool pass_rov(Announcement &ann);
    void add_policy(uint32_t);
    // Helper functions
    Announcement best_alternative_route(Announcement &ann);  // help find a good alternative route 
                                                        // (i.e. an ann from a neighbor which 
                                                        // didn't give you the attacker's announcement)
                                                        // Will return the same given ann if there is
                                                        // no better alternative
    bool is_better(Announcement &a, Announcement &b);  // Computes "is a better than b"
                                                       // This will be used to determine:
                                                       // * Replace currently used ann for a better one
                                                       // * Help make decisions on an alternative routes
    std::ostream& stream_blackholes(std:: ostream &os);
};

#endif
