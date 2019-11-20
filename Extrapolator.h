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
    bool invert;
    bool depref;
    bool origin_o;
    bool mrt_o;
    uint32_t it_size;
    uint32_t vf_as;
    ASGraph *graph;
    SQLQuerier *querier;

    Extrapolator(bool invert_results=true, 
                 bool store_depref=false, 
                 bool origin_only=false,
                 bool mrt_only=false,
                 std::string a=ANNOUNCEMENTS_TABLE,
                 std::string r=RESULTS_TABLE, 
                 std::string i=INVERSE_RESULTS_TABLE, 
                 std::string d=DEPREF_RESULTS_TABLE, 
                 std::string v=VERIFICATION_TABLE, 
                 uint32_t iteration_size=false,
                 uint32_t verification_as=false);
    ~Extrapolator();
    
    void perform_propagation(bool, size_t);
    template <typename Integer>
    void populate_blocks(Prefix<Integer>*, 
                         std::vector<Prefix<>*>*, 
                         std::vector<Prefix<>*>*);  
    void store_vf_ann(std::string,
                      uint32_t,
                      std::string);
    std::vector<uint32_t>* parse_path(std::string path_as_string); 
    bool find_loop(std::vector<uint32_t>*);
    void extrapolate_blocks(uint32_t&, 
                            int&, 
                            bool subnet, 
                            auto const& prefix_set);
    void fix_path(AS*, 
                  std::vector<uint32_t> const&, 
                  Announcement const&);
    void give_origin_to_as_path(std::vector<uint32_t>* as_path, 
                                Prefix<> prefix,
                                int64_t timestamp = 0);
    void give_ann_to_as_path(std::vector<uint32_t>* as_path, 
                             Prefix<> prefix,
                             int64_t timestamp = 0);
    void propagate_up();
    void propagate_down();
    void send_all_announcements(uint32_t asn, 
                                bool to_providers = false, 
                                bool to_peers = false, 
                                bool to_customers = false);
    void save_results(int iteration);
};
#endif
