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
                        std::string simulation_table);

    ROVppExtrapolator();
    ~ROVppExtrapolator();

    void perform_propagation(bool propogate_twice);

    void process_withdrawal(uint32_t asn, ROVppAnnouncement ann, ROVppAS *neighbor);
    void process_withdrawals(ROVppAS *as);
    bool loop_check(Prefix<> p, const ROVppAS& cur_as, uint32_t a, int d); 

    uint32_t get_priority(ROVppAnnouncement const& ann, uint32_t i);
    bool is_filtered(ROVppAS *rovpp_as, ROVppAnnouncement const& ann);

    void give_ann_to_as_path(std::vector<uint32_t>*, 
                                Prefix<> prefix, 
                                int64_t timestamp, 
                                bool hijack);

    ////////////////////////////////////////////////////////////////////
    // Overidden Methods
    ////////////////////////////////////////////////////////////////////
    void propagate_up();
    void propagate_down();
    void send_all_announcements(uint32_t asn,
                                bool to_providers = false,
                                bool to_peers = false,
                                bool to_customers = false);
    void save_results(int iteration);
};

#endif
