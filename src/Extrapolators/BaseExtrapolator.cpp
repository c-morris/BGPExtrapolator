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

#include <cmath>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <iostream>

#include "Logger.h"
#include "Extrapolators/BaseExtrapolator.h"

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::~BaseExtrapolator() {
    if(graph != NULL)
        delete graph;
    if(querier != NULL)
        delete querier;
}

/** Recursive function to break the input mrt_announcements into manageable blocks.
 *
 * @param p The current subnet for checking announcement block size
 * @param prefix_vector The vector of prefixes of appropriate size
 */
template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::populate_blocks(Prefix<>* p,
                                   std::vector<Prefix<>*>* prefix_vector,
                                   std::vector<Prefix<>*>* bloc_vector) { 
    // Find the number of announcements within the subnet
    pqxx::result r = querier->select_subnet_count(p);
    
    /** DEBUG
    std::cout << "Prefix: " << p->to_cidr() << std::endl;
    std::cout << "Count: "<< r[0][0].as<int>() << std::endl;
    */
    
    // If the subnet count is within size constraint
    if (r[0][0].as<uint32_t>() < it_size) {
        // Add to subnet block vector
        if (r[0][0].as<uint32_t>() > 0) {
            Prefix<>* p_copy = new Prefix<>(p->addr, p->netmask);
            bloc_vector->push_back(p_copy);
        }
    } else {
        // Store the prefix if there are announcements for it specifically
        pqxx::result r2 = querier->select_prefix_count(p);
        if (r2[0][0].as<uint32_t>() > 0) {
            Prefix<>* p_copy = new Prefix<>(p->addr, p->netmask);
            prefix_vector->push_back(p_copy);
        }

        // Split prefix
        // First half: increase the prefix length by 1
        uint32_t new_mask;
        if (p->netmask == 0) {
            new_mask = p->netmask | 0x80000000;
        } else {
            new_mask = (p->netmask >> 1) | p->netmask;
        }
        Prefix<>* p1 = new Prefix<>(p->addr, new_mask);
        
        // Second half: increase the prefix length by 1 and flip previous length bit
        int8_t sz = 0;
        uint32_t new_addr = p->addr;
        for (int i = 0; i < 32; i++) {
            if (p->netmask & (1 << i)) {
                sz++;
            }
        }
        new_addr |= 1UL << (32 - sz - 1);
        Prefix<>* p2 = new Prefix<>(new_addr, new_mask);

        // Recursive call on each new prefix subnet
        populate_blocks(p1, prefix_vector, bloc_vector);
        populate_blocks(p2, prefix_vector, bloc_vector);

        delete p1;
        delete p2;
    }
}

/** Parse array-like strings from db to get AS_PATH in a vector.
 *
 * libpqxx doesn't currently support returning arrays, so we need to do this. 
 * path_as_string is passed in as a copy on purpose, since otherwise it would
 * be destroyed.
 *
 * @param path_as_string the as_path as a string returned by libpqxx
 * @return as_path The AS path as vector of integers
 */
template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
std::vector<uint32_t>* BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::parse_path(std::string path_as_string) {
    std::vector<uint32_t> *as_path = new std::vector<uint32_t>;
    // Remove brackets from string
    path_as_string.erase(std::find(path_as_string.begin(), path_as_string.end(), '}'));
    path_as_string.erase(std::find(path_as_string.begin(), path_as_string.end(), '{'));

    // Fill as_path vector from parsing string
    std::string delimiter = ",";
    std::string::size_type pos = 0;
    std::string token;
    while ((pos = path_as_string.find(delimiter)) != std::string::npos) {
        token = path_as_string.substr(0,pos);
        try {
            as_path->push_back(std::stoul(token));
        } catch(...) {
            std::cerr << "Parse path error, token was: " << token << std::endl;
        }
        path_as_string.erase(0,pos + delimiter.length());
    }
    // Handle last ASN after loop
    try {
        as_path->push_back(std::stoul(path_as_string));
    } catch(...) {
        std::cerr << "Parse path error, token was: " << path_as_string << std::endl;
    }
    return as_path;
}

/** Check for loops in the path and drop announcement if they exist
 */
template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
bool BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::find_loop(std::vector<uint32_t>* as_path) {
    uint32_t prev = 0;
    bool loop = false;
    int sz = as_path->size();
    for (int i = 0; i < (sz-1) && !loop; i++) {
        prev = as_path->at(i);
        for (int j = i+1; j < sz && !loop; j++) {
            loop = as_path->at(i) == as_path->at(j) && as_path->at(j) != prev;
            prev = as_path->at(j);
        }
    }
    return loop;
}

/** Propagate announcements from customers to peers and providers ASes.
 */
template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::propagate_up() {
    size_t levels = graph->ases_by_rank->size();
    // Propagate to providers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(random);
            bool is_empty = search->second->all_anns->empty();
            if (!is_empty) {
                send_all_announcements(asn, true, false, false);
            }
        }
    }
    // Propagate to peers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(random);
            bool is_empty = search->second->all_anns->empty();
            if (!is_empty) {
                send_all_announcements(asn, false, true, false);
            }
        }
    }
}


/** Send "best" announces from providers to customer ASes. 
 */
template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::propagate_down() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = levels-1; level-- > 0;) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(random);
            bool is_empty = search->second->all_anns->empty();
            if (!is_empty) {
                send_all_announcements(asn, false, false, true);
            }
        }
    }
}

/** Save the results of a single iteration to a in-memory
 *
 * @param iteration The current iteration of the propagation
 */
template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::save_results(int iteration){
    std::ofstream outfile;
    std::string file_name = "/dev/shm/bgp/" + std::to_string(iteration) + ".csv";
    outfile.open(file_name);
    
    // Handle inverse results
    if (invert) {
        std::cout << "Saving Inverse Results From Iteration: " << iteration << std::endl;
        for (auto po : *graph->inverse_results){
            for (uint32_t asn : *po.second) {
                outfile << asn << ','
                        << po.first.first.to_cidr() << ','
                        << po.first.second << '\n';
            }
        }
        outfile.close();
        querier->copy_inverse_results_to_db(file_name);
    
    // Handle standard results
    } else {
        std::cout << "Saving Results From Iteration: " << iteration << std::endl;
        for (auto &as : *graph->ases){
            as.second->stream_announcements(outfile);
        }
        outfile.close();
        querier->copy_results_to_db(file_name);

    }
    std::remove(file_name.c_str());
    
    // Handle depref results
    if (depref) {
        std::string depref_name = "/dev/shm/bgp/depref" + std::to_string(iteration) + ".csv";
        outfile.open(depref_name);
        std::cout << "Saving Depref From Iteration: " << iteration << std::endl;
        for (auto &as : *graph->ases) {
            as.second->stream_depref(outfile);
        }
        outfile.close();
        querier->copy_depref_to_db(depref_name);
        std::remove(depref_name.c_str());
    }
}

template class BaseExtrapolator<SQLQuerier, ASGraph, Announcement, AS>;
template class BaseExtrapolator<ROVppSQLQuerier, ROVppASGraph, ROVppAnnouncement, ROVppAS>;