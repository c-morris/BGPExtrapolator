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

#ifndef ROVPP_EXTRAPOLATOR_H
#define ROVPP_EXTRAPOLATOR_H

#include "Extrapolators/BaseExtrapolator.h"
#include "Graphs/ROVppASGraph.h"
#include "Announcements/ROVppAnnouncement.h"

struct ROVppExtrapolator: public BaseExtrapolator<ROVppSQLQuerier, ROVppASGraph, ROVppAnnouncement, ROVppAS> {
    ROVppExtrapolator(std::vector<std::string> policy_tables,
                        std::string announcement_table,
                        std::string results_table,
                        std::string full_path_results_table,
                        std::string tracked_ases_table,
                        std::string simulation_table,
                        std::string config_section,
                        int exclude_as_number);

    ROVppExtrapolator();
    ~ROVppExtrapolator();

    /** Performs propagation up and down twice. First once with the Victim prefix pairs,
     * then a second time once with the Attacker prefix pairs.
     *
     * Peforms propagation of the victim and attacker prefix pairs one at a time.
     * First victims, and then attackers. The function doesn't use subnet blocks to 
     * iterate over. Instead it does the victims table all at once, then the attackers
     * table all at once.
     *
     * If iteration block sizes need to be considered, then we need to override and use the
     * perform_propagation(bool, size_t) method instead. 
     *
     * @param propagate_twice: Don't worry about it, it's ignored
     */
    void perform_propagation(bool propogate_twice);

    /** Withdraw given announcement at given neighbor.
     *
     * @param asn The AS issuing the withdrawal
     * @param ann The announcement to withdraw
     * @param neighbor The AS applying the withdraw
     */
    void process_withdrawal(uint32_t asn, ROVppAnnouncement ann, ROVppAS *neighbor);

    /** Handles processing all withdrawals at a particular AS. 
     *
     * @param as The AS that is sending out it's withdrawals
     */
    void process_withdrawals(ROVppAS *as);

    /** Check for a loop in the AS path using traceback.
     *
     * @param  p Prefix to check for
     * @param  a The ASN that, if seen, will mean we have a loop
     * @param  cur_as The current AS we are at in the traceback
     * @param  d The current depth of the search
     * @return true if a loop is detected, else false
     */
    bool loop_check(Prefix<> p, const ROVppAS& cur_as, uint32_t a, int d); 

    /** Given an announcement and index, returns priority.
    */
    uint32_t get_priority(ROVppAnnouncement const& ann, uint32_t i);

    /** Checks whether an announcement should be filtered out. 
    */
    bool is_filtered(ROVppAS *rovpp_as, ROVppAnnouncement const& ann);

    /** Seed announcement to all ASes on as_path.
     *
     * The from_monitor attribute is set to true on these announcements so they are
     * not replaced later during propagation. The ROVpp version overrides the origin
     * ASN variable at the origin AS with a flagged value for analysis.
     *
     * @param as_path Vector of ASNs for this announcement.
     * @param prefix The prefix this announcement is for.
     */
    void give_ann_to_as_path(std::vector<uint32_t>*, 
                                Prefix<> prefix, 
                                int64_t timestamp, 
                                bool hijack);

    ////////////////////////////////////////////////////////////////////
    // Overidden Methods
    ////////////////////////////////////////////////////////////////////

    /** Propagate announcements from customers to peers and providers ASes.
    */
    void propagate_up();

    /** Send "best" announces from providers to customer ASes. 
    */
    void propagate_down();

    /** Send all announcements kept by an AS to its neighbors. 
     *
     * This approximates the Adj-RIBs-out. ROVpp version simply replaces Announcement 
     * objects with ROVppAnnouncements.
     *
     * @param asn AS that is sending out announces
     * @param to_providers Send to providers
     * @param to_peers Send to peers
     * @param to_customers Send to customers
     */
    void send_all_announcements(uint32_t asn,
                                bool to_providers = false,
                                bool to_peers = false,
                                bool to_customers = false);

    /** Saves the results of the extrapolation. ROVpp version uses the ROVppQuerier.
    */
    void save_results(int iteration);
};

#endif
