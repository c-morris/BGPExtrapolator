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

#include <string>
#include <set>
#include <map>
#include <vector>
#include <random>
#include <iostream>

#include "Announcements/ROVppAnnouncement.h"
#include "ASes/BaseAS.h"

// These are the ROVppAS type flags
// They can be used to identify the type of ROVppAS
// and the in the set_rovpp_as_type function to set the type.
#define ROVPPAS_TYPE_BGP 0        // Regular BGP
#define ROVPPAS_TYPE_ROV 2        // Just standard ROV
#define ROVPPAS_TYPE_ROVPP0 7      // ROVpp 0 (Just don't send to bad neighbors)
#define ROVPPAS_TYPE_ROVPP 2      // ROVpp 0.1 (Just Blackholing)
#define ROVPPAS_TYPE_ROVPPB 3     // ROVpp 0.2 (Blackhole Announcements)
#define ROVPPAS_TYPE_ROVPPBP 4    // ROVpp 0.3 (Preventive Ann with Blackhole Ann)
#define ROVPPAS_TYPE_ROVPPBIS 5    // ROVpp 0.2bis (Blackhole Ann to Customers Only)
#define ROVPPAS_TYPE_ASPA 1024    // ASPA
#define ROVPPAS_TYPE_ASPA_ROV 1025   // ASPA
#define ROVPPAS_TYPE_ASPA_ROVPP 1026   // ASPA
#define ROVPPAS_TYPE_ASPA_ROVPPLITE 1027   // ASPA

// Special Constants 
// This is used for ROVpp 0.1+ to 
// identify blackhole announcements in the dataplane.
// We're also using this in contrl plane in pass_rov
// Constant was agreed upon with Simulation code.
#define UNUSED_ASN_FLAG_FOR_BLACKHOLES 64512  
// This flag is used to denote that along this
// path an attacker's announcement was seen, but
// the /16 is still going to be routed through here.
// Othr ROVppASes will take this into consideration 
// making decisions on what path is better.
#define ATTACKER_ON_ROUTE_FLAG 64570

class ROVppAS : public BaseAS<ROVppAnnouncement> {
public:
    // Defer processing of incoming announcements for efficiency
    std::vector<ROVppAnnouncement> *ribs_in;
    std::vector<ROVppAnnouncement> *withdrawals;

    // Maps of all announcements stored
    PrefixAnnouncementMap<ROVppAnnouncement> *loc_rib;

    std::vector<uint32_t> policy_vector;
    std::set<uint32_t> *attackers;
    std::set<uint32_t> *bad_neighbors;  // neighbors that have sent us an attacker ann

    // Announcement Tracking Member Variables
    std::set<ROVppAnnouncement> *failed_rov;  // Save dropped announcements (i.e. attacker announcements)
    std::set<ROVppAnnouncement> *passed_rov;  // History of all announcements that have passed ROV
    std::set<ROVppAnnouncement> *blackholes;  // Keep track of blackholes created
    std::set<std::pair<ROVppAnnouncement, ROVppAnnouncement>> *preventive_anns;  // Keep track of preventive announcements and their alternatives

    // Static Member Variables
    static bool graph_changed;
    
    // Constructor
    ROVppAS(uint32_t asn, uint32_t max_block_prefix_id, std::set<uint32_t> *rovpp_attackers);
    ROVppAS(uint32_t asn, uint32_t max_block_prefix_id);
    ROVppAS();

    ~ROVppAS();
    
    //****************** Announcement Handling ******************//

    /** Processes a single rovannouncement, adding it to the ASes set of announcements if appropriate.
     *
     * Approximates BGP best path selection based on rovannouncement priority.
     * Called by process_announcements and Extrapolator.give_ann_to_as_path()
     * 
     * @param ann The rovannouncement to be processed
     */ 
    void process_announcement(ROVppAnnouncement &ann, bool ran=true);

    /** Iterate through ribs_in and keep only the best. 
    */
    void process_announcements(bool ran=true);
    
    //****************** ROV Methods ******************//

    /** Checks whether or not an rovannouncement is from an attacker
     * 
     * @param  ann  rovannouncement to check if it passes ROV
     * @return bool  return false if from attacker, true otherwise
     */
    bool pass_rov(ROVppAnnouncement &ann);

    /** Checks whether or not an rovppannouncement is ASPA valid/invalid
     *
     * Those aware of how ASPA works may notice this is nothing like how ASPA
     * works. This is an approximation, assuming an attacker is not an authorized
     * neighbor. This also does not support "unknown" or "unverifiable" paths.
     * 
     * @param  ann  rovannouncement to check if it passes ASPA
     * @return bool  return false if from attacker, true otherwise
     */
    bool pass_aspa(ROVppAnnouncement &ann);

    /** Adds a policy to the policy_vector
     *
     * This function allows you to specify the policies
     * that this AS implements. The different types of policies are listed in 
     * the header of this class. 
     * 
     * @param p The policy to add. For example, ROVPPAS_TYPE_BGP (defualt), and
     */
    void add_policy(uint32_t);

    //****************** Helper Functions ******************//

    /** Add the rovannouncement to the vector of withdrawals to be processed.
     *
     *  Also remove it from the ribs_in.
     */
    void withdraw(ROVppAnnouncement &ann);
    void withdraw(ROVppAnnouncement &ann, ROVppAS &neighbor);
    
    void check_preventives(ROVppAnnouncement ann);
    void receive_announcements(std::vector<ROVppAnnouncement> &announcements);

    /** Check if a monitor announcement is already recv'd by this AS. 
     *
     * @param ann Announcement to check for. 
     * @return True if recv'd, false otherwise.
     */
    bool already_received(ROVppAnnouncement &ann);
    void clear_announcements();

    /** Will return the best alternative announcemnt if it exists. If it doesn't exist, it will return the 
     * rovannouncement it was given.
     * 
     * @param  ann An announcemnt you want to find an alternative for.
     * @return     The best alternative rovannouncement (i.e. an rovannouncement which came from a neighbor who hadn't shared
     *             an attacker announcemnt with us).
     */
    ROVppAnnouncement best_alternative_route(ROVppAnnouncement &ann);  // help find a good alternative route 
                                                        // (i.e. an ann from a neighbor which 
                                                        // didn't give you the attacker's announcement)
                                                        // Will return the same given ann if there is
                                                        // no better alternative
    
    /**
     * [ROVppAS::stream_blacklist description]
     * @param  os [description]
     * @return    [description]
     */
    std::ostream& stream_blackholes(std:: ostream &os);
};
#endif
