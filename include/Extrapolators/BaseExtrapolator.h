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

#include "SQLQueriers/EZSQLQuerier.h"
#include "Graphs/EZASGraph.h"
#include "Announcements/EZAnnouncement.h"
#include "ASes/EZAS.h"

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

    /** Parse array-like strings from db to get AS_PATH in a vector.
     *
     * libpqxx doesn't currently support returning arrays, so we need to do this. 
     * path_as_string is passed in as a copy on purpose, since otherwise it would
     * be destroyed.
     *
     * @param path_as_string the as_path as a string returned by libpqxx
     * @return as_path The AS path as vector of integers
     */
    std::vector<uint32_t>* parse_path(std::string path_as_string); 

    /** Check for loops in the path and drop announcement if they exist
    */
    bool find_loop(std::vector<uint32_t>*);

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
};
#endif
