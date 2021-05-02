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

#define DEFAULT_RANDOM_TIEBRAKING true
#define DEFAULT_STORE_RESULTS true
#define DEFAULT_STORE_INVERT_RESULTS false
#define DEFAULT_STORE_DEPREF_RESULTS false

#define DEFAULT_ORIGIN_ONLY false

#define DEFAULT_MAX_THREADS 0

#include <vector>
#include <bits/stdc++.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <thread>
#include <stdio.h>
#include <dirent.h>
#include <semaphore.h>
#include <cstdint>
#include <vector>

#include "ASes/AS.h"
#include "Graphs/ASGraph.h"
#include "Announcements/Announcement.h"
#include "Prefix.h"
#include "SQLQueriers/SQLQuerier.h"
#include "TableNames.h"

#include "SQLQueriers/ROVppSQLQuerier.h"
#include "Graphs/ROVppASGraph.h"

#include "SQLQueriers/EZSQLQuerier.h"
#include "Graphs/EZASGraph.h"
#include "Announcements/EZAnnouncement.h"
#include "ASes/EZAS.h"

#include "SQLQueriers/ROVSQLQuerier.h"
#include "Graphs/ROVASGraph.h"
#include "Announcements/ROVAnnouncement.h"
#include "ASes/ROVAS.h"

/** README. SERIOUSLY README
 * Trust me you need to read this.
 * 
 * The Extrapolator family is heavily templated so we can give variables very specific types that we don't want to mix.
 * The ROVppExtrapolator only deals with ROVppAnnouncements and ROVppASes. Thus, the type of those variables is templated.
 * This makes putting ASes into data structures much easier and avoids constant casting.
 * 
 * The Base Extrapolator contains functionality that ALL extrapolators should have, and should be able to call. 
 * For example, the ROVppExtrapolator should not be able to call extrapolate_blocks becuase it does not handle data in that way.
 * 
 * However, extrapolators all need (or can) call parse_path, find_loop, etc...
 * In addition, DO NOT create an object of any base template  class!!! These template base classes are meant for inheritence ONLY!!!
 * 
 * Also, if you have never seen templating in c++ (or generic in Java -> the two are different but pretty similar in concept), you want to do a bit of research.
 * Basically when you create a template class and ask it to create it with specified types (e.g. BaseExtrapolator<AS, Announcement, etc..>) you define a whole new
 *      type at compile time. This means that BaseExtrapolator<AS, Announcement, ...> and BaseExtrapolaor<EZAS, EZAnnouncement, ...> are as different as int and double
 * You CANNOT create a data structure with a template type and expect to start mixing types together
 * Template types are a fancy copy and paste that occurs at compile time to generate new data types.
 * 
 * If you need to ADD ANOTHER EXTRAPOLATOR TYPE, here is what you need to do:
 *  - Ideally, create new classes for the graph, announcement, querier, and as, that follow the instructions there
 *  - Extend a template class (as of writing this that would be either the BaseExtrapolator or Blocked Extrapolator) with template types of the classes you just made
 *  - write out the functions, etc...
 *  - IMPORTANT: at the bottom of EACH PARENT template class .cpp file you need to put in this line (replace the tpyes with your new additions):
 *      template class BaseExtrapolator<SQLQuerier, ASGraph, Announcement, AS>;
 *      This is an odd nuance with c++ for when you want to have a .cpp file for your templated class
 * 
 * UNDEFINED REFRENCE
 *      If you are getting this error, and it involves an extended type from one of the templated classes, 
 *          you need to add the template class line of code above in the .cpp file of each parent template base classes (parents, grand parents, etc... All the way to the top).
 * 
 * If you need to ADD ANOTHER TEMPLATE EXTRAPOLATOR TYPE:
 *  - It works just as you think, template the class with the generic types and extend another template class (using the template types)
 *  - THE CONSTRUCTOR MUST BE DEFINED IN THE HEADER!!!! Yet another odd nuance with c++ inheritence with template classes (fix: define everything in the header, we ain't doing that)
 *  - HOWEVER, A NUANCE is that in the .cpp file for this template class, if you want to refrence any member variables in the parent class, you HAVE TO use this-> to get to it
 *      This is an odd nuance with c++ for making a template class that extends another template class, just roll with it, I have spent a bit of time trying to work
 *      around this, there is nothing simple that would fix this unless we start implementing in the header file.
 *  - An example of this is Blocked Extrapolator
 *  - When writing this, USE THE TEMPLATE TYPES (AnnouncementType, ASType, etc...). This will make it so when you make a class that uses a version of the, for example, AS class
 *      it will paste in the particular inherited class in compilation.
 *  - When writing these template classes, you can assume that the type of classes (ASType, AnnouncementType) are at minimum the most basic version
 * 
 * On the off chance I missed something here, look that the classes that are already here. In terms of inheritence/family structure, what you want is probably here already
 * 
 * As of writing this, here is the family:
 * 
 *          Base
 *        /      \
 *      ROV      Blocked
 *               /     \
 *              EZ    Vanilla 
 */
template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
class BaseExtrapolator {
public:
    GraphType *graph;
    SQLQuerierType *querier;

    bool random_tiebraking;    // If randomness is enabled
    bool store_results; // If results are enabled
    bool store_invert_results; // If inverted results are enabled
    bool store_depref_results; // If depref results are enabled
    std::vector<uint32_t> *full_path_asns; // Limit output to these ASNs
    sem_t worker_thread_count; // Worker thread semaphore
    int max_workers;           // Max number of worker threads that can run concurrently
    sem_t csvs_written;        // Semaphore to delay saving to the database
    bool origin_only;          // Only seed at the origin AS

    BaseExtrapolator(bool random_tiebraking,
                        bool store_results, 
                        bool store_invert_results, 
                        bool store_depref_results,
                        bool origin_only,
                        std::vector<uint32_t> *full_path_asns,
                        int max_threads) {

        this->random_tiebraking = random_tiebraking;       // True to enable random tiebreaks
        this->store_results = store_results; // True to store regular results
        this->store_invert_results = store_invert_results; // True to store the results inverted
        this->store_depref_results = store_depref_results; // True to store the second best ann for depref
        this->origin_only = origin_only;                   // True to only seed at the origin AS
        this->full_path_asns = full_path_asns;

        // Get the number of CPU cores available
        int cpus = std::thread::hardware_concurrency();
        if (max_threads <= 0 || max_threads >= cpus)
        {
            // If a user doesn't provide a number of max threads (or if it's invalid),
            // max_workers is one minus the number of CPU cores available
            max_workers = cpus > 1 ? cpus - 1 : 1;
        } else {
            // Set max_workers to max_threads
            // if max_threads is less than total number of threads and if it's more than 0
            max_workers = max_threads;
        }

        // Init worker thread semaphore
        sem_init(&worker_thread_count, 0, max_workers);

        // Init csv semaphore to 0. That way the next iteration starts only after all csvs were written
        sem_init(&csvs_written, 0, 0);
        
        // The child will initialize these properly right after this constructor returns
        // That way they can give the variable a proper type
        graph = NULL;
        querier = NULL;
    }

    /**
     * Default constructor. Leaves the graph and querier NULL to be created by the inherited child. 
     * 
     * Default Values:
     *  - random_tiebraking = true
     *  - store_invert_results = true
     *  - store_depref_results = false
     */
    BaseExtrapolator() : BaseExtrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS) { }

    virtual ~BaseExtrapolator();

    /** Parse array-like strings from db to get AS_PATH in a vector.
     *
     * libpqxx doesn't currently support returning arrays, so we need to do this. 
     * path_as_string is passed in as a copy on purpose, since otherwise it would
     * be destroyed.
     *
     * @param path_as_string the as_path as a string returned by libpqxx
     * @return as_path The AS path as vector of integers
     */
    virtual std::vector<uint32_t>* parse_path(std::string path_as_string); 

    /** Check for loops in the path and drop announcement if they exist
    */
    virtual bool find_loop(std::vector<uint32_t>* as_path);

    /** Propagate announcements from customers to peers and providers ASes.
    */
    virtual void propagate_up();

    /** Send "best" announces from providers to customer ASes. 
    */
    virtual void propagate_down();

    
    /** Send all announcements kept by an AS to its neighbors. 
     *
     * This approximates the Adj-RIBs-out. 
     *
     * @param asn AS that is sending out announces
     * @param to_providers Send to providers
     * @param to_peers Send to peers
     * @param to_customers Send to customers
     */
    virtual void send_all_announcements(uint32_t asn, 
                                        bool to_providers = false, 
                                        bool to_peers = false, 
                                        bool to_customers = false) = 0;

    /** Save the results of a single iteration to a in-memory
     *
     * @param iteration The current iteration of the propagation
     */
    virtual void save_results(int iteration);

    /** Thread function to save results
     *
     * @param iteration The current iteration of the propagation
     */
    virtual void save_results_thread(int iteration, int thread_num, int num_threads);

    /** Save results only at a particular AS
     *
     * These results will also contain the full AS_PATH computed by tracing back
     * the announcements to their origin. These results are not inverted.
     *
     * @param asn AS to save results for
     */
    virtual void save_results_at_asn(uint32_t asn);

    /** Return the AS_PATH of an Announcement.
     *
     * @param ann Announcement to determine the path of
     * @param asn AS number of the AS to start at
     * @return The AS_PATH as a string, formatted as a postgres array literal
     */
    virtual std::string stream_as_path(AnnouncementType ann, uint32_t asn);

};
#endif
