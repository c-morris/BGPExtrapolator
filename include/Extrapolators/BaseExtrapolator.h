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

#ifndef BASE_EXTRAPOLATOR_H
#define BASE_EXTRAPOLATOR_H

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

#include "ASes/AS.h"
#include "Graphs/ASGraph.h"
#include "Announcements/Announcement.h"
#include "Prefix.h"
#include "SQLQueriers/SQLQuerier.h"
#include "TableNames.h"

#include "SQLQueriers/ROVppSQLQuerier.h"
#include "Graphs/ROVppASGraph.h"

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
class BaseExtrapolator {
public:
    bool random;            // If randomness is enabled
    bool invert;            // If inverted results are enabled
    bool depref;            // If depref results are enabled
    uint32_t it_size;       // # of announcements per iteration
    GraphType *graph;
    SQLQuerierType *querier;

    BaseExtrapolator(bool random_b=true,
                 bool invert_results=true, 
                 bool store_depref=false, 
                 std::string a=ANNOUNCEMENTS_TABLE,
                 std::string r=RESULTS_TABLE, 
                 std::string i=INVERSE_RESULTS_TABLE, 
                 std::string d=DEPREF_RESULTS_TABLE, 
                 uint32_t iteration_size=false) {

        random = random_b;                          // True to enable random tiebreaks
        invert = invert_results;                    // True to store the results inverted
        depref = store_depref;                      // True to store the second best ann for depref
        it_size = iteration_size;                   // Number of prefix to be precessed per iteration
        
        // The child will initialize these properly right after this constructor returns
        graph = NULL;
        querier = NULL;
    }

    virtual ~BaseExtrapolator();

    // Yes both versions use the same name for the function, but we DO NOT want the ROV version to call the 
    // vanilla version (they have different parameters)! That would be bad. To prevent this, the function is just split down to the two
    // versions
    // void perform_propagation();
    
    void populate_blocks(Prefix<>*, 
                         std::vector<Prefix<>*>*, 
                         std::vector<Prefix<>*>*);
    std::vector<uint32_t>* parse_path(std::string path_as_string); 

    // ROV version is not interested in using this function... Don't want this being called in the ROV version
    // void extrapolate_blocks(uint32_t&, 
    //                         int&, 
    //                         bool subnet, 
    //                         auto const& prefix_set);

    bool find_loop(std::vector<uint32_t>*);
    virtual void propagate_up();
    virtual void propagate_down();

    /** Seed announcement on all ASes on as_path. 
     *
     * The from_monitor attribute is set to true on these announcements so they are
     * not replaced later during propagation. 
     * 
     * Must be implemented by child class!
     *
     * @param as_path Vector of ASNs for this announcement.
     * @param prefix The prefix this announcement is for.
     */
    // The AS versions and the ROV versions have different perameters for this. 
    // Inheritance is only harmful, as the ROV version would not want to call this, and it would be bad to do so
    // virtual void give_ann_to_as_path(std::vector<uint32_t>* as_path, 
    //                          Prefix<> prefix,
    //                          int64_t timestamp = 0) = 0;

    //Children need to handle this one
    virtual void send_all_announcements(uint32_t asn, 
                                bool to_providers = false, 
                                bool to_peers = false, 
                                bool to_customers = false) = 0;

    virtual void save_results(int iteration);

    //????
    void invert_results(void);
};
#endif
