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
#include <vector>
#include <thread>
#include <semaphore.h>

#include "Logger.h"
#include "Extrapolators/BaseExtrapolator.h"

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::~BaseExtrapolator() {
    if(graph != NULL)
        delete graph;
    if(querier != NULL)
        delete querier;
    sem_destroy(&worker_thread_count);
}

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

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::propagate_up() {
    size_t levels = graph->ases_by_rank->size();
    // Propagate to providers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(random_tiebraking);
            bool is_empty = search->second->all_anns->empty();
            if (!is_empty) {
                send_all_announcements(asn, true, false, false);
            }
        }
    }
    // Propagate to peers
    // When propagating to peers,
    // all ASes may not have processed all incoming announcements after the function completes.
    // Those announcements will be processed after propagate_down()
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(random_tiebraking);
            bool is_empty = search->second->all_anns->empty();
            if (!is_empty) {
                send_all_announcements(asn, false, true, false);
            }
        }
    }
}


template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::propagate_down() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = levels-1; level-- > 0;) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(random_tiebraking);
            bool is_empty = search->second->all_anns->empty();
            if (!is_empty) {
                send_all_announcements(asn, false, false, true);
            }
        }
    }
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::save_results_thread(int iteration, int thread_num, int num_threads){
    // Decrement semaphore to limit the number of concurrent threads
    sem_wait(&worker_thread_count);
    int counter = thread_num;
    // Need a copy of the querier to make a new db connection to avoid resource conflicts 
    SQLQuerierType querier_copy(*querier);
    querier_copy.open_connection();
    std::ofstream outfile;
    std::string file_name = "/dev/shm/bgp/" + std::to_string(iteration) + "_" + std::to_string(thread_num) + ".csv";
    outfile.open(file_name);

    // Handle standard results
    if (store_results) {
        for (auto &as : *graph->ases){
            if (counter++ % num_threads == 0) {
                as.second->stream_announcements(outfile);
            }
        }
        outfile.close();
        querier_copy.copy_results_to_db(file_name);
    }
    
    // Handle inverse results
    if (store_invert_results) {
        for (auto po : *graph->inverse_results){
            // The results are divided into num_threads CSVs. For example, with 
            // four threads, this loop will save every fourth item in the loop.
            if (counter++ % num_threads == 0) {
                for (uint32_t asn : *po.second) {
                    outfile << asn << ','
                            << po.first.first.to_cidr() << ','
                            << po.first.second << '\n';
                }
            }
        }
        outfile.close();
        querier_copy.copy_inverse_results_to_db(file_name);
    
    }    

    std::remove(file_name.c_str());

    // Handle depref results
    if (store_depref_results) {
        std::string depref_name = "/dev/shm/bgp/depref" + std::to_string(iteration) + "_" + std::to_string(thread_num) + ".csv";
        outfile.open(depref_name);
        for (auto &as : *graph->ases) {
            if (counter++ % num_threads == 0) {
                as.second->stream_depref(outfile);
            }
        }
        outfile.close();
        querier_copy.copy_depref_to_db(depref_name);
        std::remove(depref_name.c_str());
    }

    // Handle full_path results
    if (full_path_asns != NULL) {
        for (uint32_t asn : *full_path_asns){
            if (counter++ % num_threads == 0) {
                this->save_results_at_asn(asn);
            }
        }
    }

    querier_copy.close_connection();
    sem_post(&worker_thread_count);
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::save_results(int iteration){
    if (store_invert_results) {
        std::cout << "Saving Inverse Results From Iteration: " << iteration << std::endl;
    } else {
        std::cout << "Saving Results From Iteration: " << iteration << std::endl;
    }
    if (store_depref_results) {
        std::cout << "Saving Depref Results From Iteration: " << iteration << std::endl;
    }
    std::vector<std::thread> threads;
    int cpus = std::thread::hardware_concurrency();
    // Ensure we have at least one worker even when only one cpu core is avaliable
    int max_workers = cpus > 1 ? cpus - 1 : 1;
    if (max_workers > 1) {
        for (int i = 0; i < max_workers; i++) {
            // Start the worker threads
            threads.push_back(std::thread(&BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::save_results_thread, this, iteration, i, max_workers));
        }
        for (size_t i = 0; i < threads.size(); i++) {
            // Note, this could be done slightly faster, technically we can start
            // propagating the next iteration once the CSVs are saved, we don't
            // need to wait for the database insertion, but the speedup likely
            // isn't worth the added complexity at this point.
            threads[i].join();
        }
    } else {
        this->save_results_thread(iteration, 0, 1);
    }
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
void BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::save_results_at_asn(uint32_t asn){
    auto search = graph->ases->find(asn); 
    if (search == graph->ases->end()) {
        // If the asn does not exist, return
        return;
    }
    ASType &as = *search->second;
    std::ofstream outfile;
    std::string file_name = "/dev/shm/bgp/as" + std::to_string(asn) + ".csv";
    outfile.open(file_name);
    if (as.asn == asn) {
        for (auto &ann : *as.all_anns) {
            AnnouncementType &a = ann.second;
            outfile << asn << ',' << a.prefix.to_cidr() << ',' << a.origin << ',' << a.received_from_asn << ',' << a.tstamp << ",\"" << this->stream_as_path(a, asn) << "\"\n";
        }
    }
    outfile.close();
    this->querier->copy_single_results_to_db(file_name);
    std::remove(file_name.c_str());
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
std::string BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::stream_as_path(AnnouncementType &ann, uint32_t asn){
    int depth = 0;
    int max_depth = 1023; // Prevent infinite loops
    std::stringstream as_path;
    as_path << '{' << asn << ',';

    // Trace back until one of these conditions
    // 1. The origin is the received from ASN
    // 2. The received from ASN does not exist in the graph
    // 3. Max depth is exceeded--- this likely indicates a loop in the topology
    while (ann.origin != ann.received_from_asn && 
           graph->ases->find(ann.received_from_asn) != graph->ases->end() &&
           depth < max_depth) {
        depth++;
        as_path << ann.received_from_asn << ',';
        ann = graph->ases->find(ann.received_from_asn)->second->all_anns->find(ann.prefix)->second;
    }
    // Add the last AS on the path
    as_path << ann.received_from_asn << '}';
    return as_path.str();
}

//We love C++ class templating. Please find another way to do this. I want to be wrong.
template class BaseExtrapolator<SQLQuerier, ASGraph, Announcement, AS>;
template class BaseExtrapolator<EZSQLQuerier, EZASGraph, EZAnnouncement, EZAS>;
template class BaseExtrapolator<ROVppSQLQuerier, ROVppASGraph, ROVppAnnouncement, ROVppAS>;
