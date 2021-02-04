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

#include <vector>
#include <bits/stdc++.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <dirent.h>

#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Prefix.h"
#include "SQLQuerier.h"
#include "TableNames.h"

class Extrapolator {
public:
    bool random;            // If randomness is enabled
    bool invert;            // If inverted results are enabled
    bool depref;            // If depref results are enabled
    uint32_t it_size;       // # of announcements per iteration
    ASGraph *graph;
    SQLQuerier *querier;
    
    Extrapolator(ASGraph*, 
                 SQLQuerier*, 
                 uint32_t=false);
 
    Extrapolator(bool random=true,
                 bool invert_results=true, 
                 bool store_depref=false, 
                 std::string a=ANNOUNCEMENTS_TABLE,
                 std::string r=RESULTS_TABLE,
                 std::string i=INVERSE_RESULTS_TABLE,
                 std::string d=DEPREF_RESULTS_TABLE,
                 uint32_t iteration_size=false,
                 std::string config_section=DEFAULT_QUERIER_CONFIG_SECTION);
    virtual ~Extrapolator();

    virtual void perform_propagation();
    template <typename Integer>
    void populate_blocks(Prefix<Integer>*,
                         std::vector<Prefix<>*>*,
                         std::vector<Prefix<>*>*);
    std::vector<uint32_t>* parse_path(std::string path_as_string);
    void extrapolate_blocks(uint32_t&,
                            int&,
                            bool subnet,
                            auto const& prefix_set);
    bool find_loop(std::vector<uint32_t>*);
    virtual void propagate_up();
    virtual void propagate_down();
    void give_ann_to_as_path(std::vector<uint32_t>* as_path,
                             Prefix<> prefix,
                             int64_t timestamp = 0,
                             uint32_t roa_validity = 2);
    virtual void send_all_announcements(uint32_t asn,
                                bool to_providers = false,
                                bool to_peers = false,
                                bool to_customers = false);
    virtual void save_results(int iteration);
    void invert_results(void);
};
#endif
