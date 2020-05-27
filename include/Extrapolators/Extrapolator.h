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

#ifndef EXTRAPOLATOR_H
#define EXTRAPOLATOR_H

#include "Extrapolators/BaseExtrapolator.h"

class Extrapolator : public BaseExtrapolator<SQLQuerier, ASGraph, Announcement, AS> {
public:
    Extrapolator(bool random=true,
                 bool invert_results=true, 
                 bool store_depref=false, 
                 std::string a=ANNOUNCEMENTS_TABLE,
                 std::string r=RESULTS_TABLE, 
                 std::string i=INVERSE_RESULTS_TABLE, 
                 std::string d=DEPREF_RESULTS_TABLE, 
                 uint32_t iteration_size=false);    

    /** Performs all tasks necessary to propagate a set of announcements given:
     *      1) A populated mrt_announcements table
     *      2) A populated customer_provider table
     *      3) A populated peers table
     *
     * @param test
     * @param iteration_size number of rows to process each iteration, rounded down to the nearest full prefix
     * @param max_total maximum number of rows to process, ignored if zero
     */
    void perform_propagation();

    void extrapolate_blocks(uint32_t &announcement_count, 
                            int &iteration, 
                            bool subnet, 
                            auto const& prefix_set);

    void give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp = 0);
    void send_all_announcements(uint32_t asn, bool to_providers = false, bool to_peers = false, bool to_customers = false);
};

#endif