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
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(random_tiebraking);
            bool is_empty = search->second->all_anns->empty();

            std::cout << "*Propagating to peers from AS# " << asn << "*" << std::endl;
            std::cout << "*is_empty: " << is_empty << "*" << std::endl;
            for (uint32_t peer_asn : *search->second->peers) {
            //auto *recving_as = this->graph->ases->find(customer_asn)->second;
                std::cout << "Peer: " << peer_asn << std::endl;
            }


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
    
    // Handle standard results
    } else {
        for (auto &as : *graph->ases){
            if (counter++ % num_threads == 0) {
                as.second->stream_announcements(outfile);
            }
        }
        outfile.close();
        querier_copy.copy_results_to_db(file_name);

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

//We love C++ class templating. Please find another way to do this. I want to be wrong.
template class BaseExtrapolator<SQLQuerier, ASGraph, Announcement, AS>;
template class BaseExtrapolator<EZSQLQuerier, EZASGraph, EZAnnouncement, EZAS>;
template class BaseExtrapolator<ROVppSQLQuerier, ROVppASGraph, ROVppAnnouncement, ROVppAS>;
