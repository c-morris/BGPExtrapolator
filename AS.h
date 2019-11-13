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

#ifndef AS_H
#define AS_H

#define AS_REL_PROVIDER 0
#define AS_REL_PEER 1
#define AS_REL_CUSTOMER 2

#include <string>
#include <set>
#include <map>
#include <vector>
#include <random>
#include <iostream>

#include "Announcement.h"
#include "Prefix.h"

class AS {
public:
    uint32_t asn;       // Autonomous System Number
    bool visited;       // Marks something
    int rank;           // Rank in ASGraph heirarchy for propagation
    
    // Defer processing of incoming announcements for efficiency
    std::vector<Announcement> *incoming_announcements;
    // Maps of all announcements stored
    std::map<Prefix<>, Announcement> *all_anns;
    std::map<Prefix<>, Announcement> *depref_anns;
    // Stores AS Relationships
    std::set<uint32_t> *providers; 
    std::set<uint32_t> *peers; 
    std::set<uint32_t> *customers; 
    // Pointer to inverted results map for efficiency
    std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results; 
    // If this AS represents multiple ASes, it's "members" are listed here (Supernodes)
    std::vector<uint32_t> *member_ases;
    // Assigned and used in Tarjan's algorithm
    int index;
    int lowlink;
    bool onStack;
    
    // Constructor
    AS(uint32_t myasn=0, 
        std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results=NULL,
        std::set<uint32_t> *prov=NULL, 
        std::set<uint32_t> *peer=NULL,
        std::set<uint32_t> *cust=NULL);
    ~AS();
    
    void add_neighbor(uint32_t asn, int relationship);
    void remove_neighbor(uint32_t asn, int relationship);
    void receive_announcements(std::vector<Announcement> &announcements);
    void process_announcement(Announcement &ann);
    void clear_announcements();
    bool already_received(Announcement &ann);
    void delete_ann(Announcement &ann);
    void printDebug();
    void process_announcements();
    void swap_inverse_result(std::pair<Prefix<>,uint32_t> old, 
                             std::pair<Prefix<>,uint32_t> current);
    friend std::ostream& operator<<(std::ostream &os, const AS& as);
    std::ostream& stream_announcements(std:: ostream &os);
    std::ostream& stream_depref(std:: ostream &os);
private:
    // Random Number Generator
    std::minstd_rand ran_bool;
};
#endif
