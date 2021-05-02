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

#define TEST_RESULTS_TABLE "test_extrapolate_blocks_results"
#define TEST_ANNOUNCEMENTS_TABLE "mrt_announcements_test"

#include <iostream>
#include <cstdint>
#include <vector>

#include "Extrapolators/Extrapolator.h"

/** Unit tests for Extrapolator.h and Extrapolator.cpp
 */


/** It is worth having this test since the constructor is changed so often.
 */
bool test_Extrapolator_constructor() {
    Extrapolator e = Extrapolator();
    if (e.graph == NULL) { return false; }
    return true;
}

/** Test the loop detection in input MRT AS paths.
 */
bool test_find_loop() {
    Extrapolator e = Extrapolator();
    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(1);
    as_path->push_back(2);
    as_path->push_back(3);
    as_path->push_back(1);
    as_path->push_back(4);
    bool loop = e.find_loop(as_path);
    if (!loop) {
        std::cerr << "Loop detection failed." << std::endl;
        return false;
    }
    
    // Prepending handling check
    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(1);
    as_path_b->push_back(2);
    as_path_b->push_back(2);
    as_path_b->push_back(3);
    as_path_b->push_back(4);
    loop = e.find_loop(as_path_b);
    if (loop) {
        std::cerr << "Loop prepending correctness failed." << std::endl;
        return false;
    }
    return true;
}

// Test parse_path function
bool test_parse_path() {
    Extrapolator e = Extrapolator();
    std::vector<uint32_t> *as_path = e.parse_path("{63027,32899,12083,1299,14522}");
    std::vector<uint32_t> true_as_path {63027,32899,12083,1299,14522};

    if (as_path->size() != true_as_path.size()) {
        std::cerr << "Parse path failed." << std::endl;
        return false;
    }

    // Check that every AS in as_path is correct
    for (unsigned int i = 0; i < as_path->size(); i++) {
        if (as_path->at(i) != true_as_path.at(i)) {
            std::cerr << "Parse path failed." << std::endl;
            return false;
        }
    }
    
    return true;
}

/** Test seeding the graph with announcements from monitors. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  The test path vect is [3, 2, 5]. 
 */
bool test_give_ann_to_as_path() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);

    // Test that monitor annoucements were received
    if(!(e.graph->ases->find(2)->second->all_anns->find(p)->second.from_monitor &&
         e.graph->ases->find(3)->second->all_anns->find(p)->second.from_monitor &&
         e.graph->ases->find(5)->second->all_anns->find(p)->second.from_monitor)) {
        std::cerr << "Monitor flag failed." << std::endl;
        return false;
    }

    // Test announcement priority calculation
    if (e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400) {
        std::cerr << "Priority calculation failed." << std::endl;
        return false;
    }

    // Test that only path received the announcement
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
        e.graph->ases->find(2)->second->all_anns->size() == 1 &&
        e.graph->ases->find(3)->second->all_anns->size() == 1 &&
        e.graph->ases->find(4)->second->all_anns->size() == 0 &&
        e.graph->ases->find(5)->second->all_anns->size() == 1 &&
        e.graph->ases->find(6)->second->all_anns->size() == 0)) {
        std::cerr << "MRT overseeding check failed." << std::endl;
        return false;
    }

    // Test timestamp tie breaking
    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(1);
    as_path_b->push_back(2);
    as_path_b->push_back(4);
    as_path_b->push_back(4);
    e.give_ann_to_as_path(as_path_b, p, 1);

    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.tstamp != 1) {
        return false;
    }
    
    // Test prepending calculation
    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 298) {
        std::cout << e.graph->ases->find(2)->second->all_anns->find(p)->second.priority << std::endl;
        return false;
    }

    delete as_path;
    delete as_path_b;
    return true;
}

/** Test seeding the graph with announcements from monitors with origin only option enabled. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  The test path vect is [3, 2, 5]. 
 *  When AS 5 is the origin, an announcement should only be seeded at 5
 */
bool test_give_ann_to_as_path_origin_only() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 0, true, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);

    // Test that monitor annoucements were received
    if(!e.graph->ases->find(5)->second->all_anns->find(p)->second.from_monitor) {
        std::cerr << "Monitor flag failed." << std::endl;
        return false;
    }

    // Test announcement priority calculation
    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400) {
        std::cerr << "Priority calculation failed." << std::endl;
        return false;
    }

    // Test that only path received the announcement
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
        e.graph->ases->find(2)->second->all_anns->size() == 0 &&
        e.graph->ases->find(3)->second->all_anns->size() == 0 &&
        e.graph->ases->find(4)->second->all_anns->size() == 0 &&
        e.graph->ases->find(5)->second->all_anns->size() == 1 &&
        e.graph->ases->find(6)->second->all_anns->size() == 0)) {
        std::cerr << "MRT overseeding check failed." << std::endl;
        return false;
    }
    
    delete as_path;
    return true;
}

/** Test propagating up without multihomed support in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2---3
 *   /|    \
 *  4 5--6  7
 * 
 *  Starting propagation at 5, only 4 and 7 should not see the announcement.
 */
bool test_propagate_up_no_multihomed() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 0, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(7, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 7, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(5)->second->process_announcement(ann, true);
    e.propagate_up();

    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->all_anns->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 0 &&
          e.graph->ases->find(5)->second->all_anns->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 1 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Propagate up failed." << std::endl;
        return false;
    }

    // Check propagation priority calculation
    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 290 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 289 ||
        e.graph->ases->find(6)->second->all_anns->find(p)->second.priority != 189 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 288 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 188) {
        std::cerr << "Propagted priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test propagating up with automatic multihomed mode (default behaviour) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2---3
 *   /|    \
 *  4 5--6  7
 *
 *  Starting propagation at 5, everyone but 4 and 7 should see the announcement.
 */
bool test_propagate_up() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(7, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 7, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(5)->second->process_announcement(ann, true);
    e.propagate_up();

    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->all_anns->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 0 &&
          e.graph->ases->find(5)->second->all_anns->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 1 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "test_propagate_up_multihomed_automatic failed... Not all ASes have refrence when they should.." << std::endl;
        return false;
    }
    
    // Check propagation priority calculation
    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 290 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 289 ||
        e.graph->ases->find(6)->second->all_anns->find(p)->second.priority != 189 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 288 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 188) {
        std::cerr << "Propagted priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test propagating up (no propagation from multihomed - mode 1) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *     1
 *    /|
 *   4 2
 *    \|   
 *     3--5
 *
 *  Starting propagation at 3, only 3 should see the announcement.
 */
bool test_propagate_up_multihomed_standard() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 2, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 4, AS_REL_PROVIDER);
    e.graph->add_relationship(4, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 5, AS_REL_PEER);
    e.graph->add_relationship(5, 3, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(3)->second->process_announcement(ann, true);
    e.propagate_up();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
        e.graph->ases->find(2)->second->all_anns->size() == 0 &&
        e.graph->ases->find(3)->second->all_anns->size() == 1 &&
        e.graph->ases->find(4)->second->all_anns->size() == 0 &&
        e.graph->ases->find(5)->second->all_anns->size() == 0)) {
        
        std::cerr << "test_propagate_up_multihomed_standard failed... Not all ASes have refrence when they should.." << std::endl;
        return false;
    }
    
    if (e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 290) {
        std::cerr << "Propagted priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test propagating up (only propagation to peers from multihomed - mode 2) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *     1
 *    /|
 *   4 2
 *    \|   
 *     3--5
 *
 *  Starting propagation at 3, 3 and 5 should see the announcement.
 */
bool test_propagate_up_multihomed_peer_mode() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE,
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 3, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 4, AS_REL_PROVIDER);
    e.graph->add_relationship(4, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 5, AS_REL_PEER);
    e.graph->add_relationship(5, 3, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(3)->second->process_announcement(ann, true);
    e.propagate_up();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
        e.graph->ases->find(2)->second->all_anns->size() == 0 &&
        e.graph->ases->find(3)->second->all_anns->size() == 1 &&
        e.graph->ases->find(4)->second->all_anns->size() == 0 &&
        e.graph->ases->find(5)->second->all_anns->size() == 1)) {
        
        std::cerr << "test_propagate_up_multihomed_peer_mode failed... Not all ASes have refrence when they should.." << std::endl;
        return false;
    }
    
    if (e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 290 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 189) {
        std::cerr << "Propagted priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test propagating down without multihomed support in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Starting propagation at 2, 4 and 5 should see the announcement.
 */
bool test_propagate_down_no_multihomed() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 0, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(2)->second->process_announcement(ann, true);
    e.propagate_down();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
        e.graph->ases->find(2)->second->all_anns->size() == 1 &&
        e.graph->ases->find(3)->second->all_anns->size() == 0 &&
        e.graph->ases->find(4)->second->all_anns->size() == 1 &&
        e.graph->ases->find(5)->second->all_anns->size() == 1 &&
        e.graph->ases->find(6)->second->all_anns->size() == 0)) {
        std::cerr << "test_propagate_down_no_multihomed failed... Not all ASes have refrence when they should.." << std::endl;
        return false;
    }
    
    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 290 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 89 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 89) {
        std::cerr << "Propagated priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test propagating down without multihomed support in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Starting propagation at 1, everyone but 3 and 6 should see the announcement.
 */
bool test_propagate_down_no_multihomed2() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 0, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(1)->second->process_announcement(ann, true);
    e.propagate_down();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 1 &&
        e.graph->ases->find(2)->second->all_anns->size() == 1 &&
        e.graph->ases->find(3)->second->all_anns->size() == 0 &&
        e.graph->ases->find(4)->second->all_anns->size() == 1 &&
        e.graph->ases->find(5)->second->all_anns->size() == 1 &&
        e.graph->ases->find(6)->second->all_anns->size() == 0)) {
        
        std::cerr << "test_propagate_down_no_multihomed2 failed... Not all ASes have refrence when they should.." << std::endl;
        return false;
    }

    if (e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 290 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 89 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 88 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 88) {
        std::cerr << "Propagated priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test propagating down with automatic multihomed mode (default behaviour) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Starting propagation at 2, 4 and 5 should see the announcement.
 */
bool test_propagate_down() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(2)->second->process_announcement(ann, true);
    e.propagate_down();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
        e.graph->ases->find(2)->second->all_anns->size() == 1 &&
        e.graph->ases->find(3)->second->all_anns->size() == 0 &&
        e.graph->ases->find(4)->second->all_anns->size() == 1 &&
        e.graph->ases->find(5)->second->all_anns->size() == 1 &&
        e.graph->ases->find(6)->second->all_anns->size() == 0)) {
        std::cerr << "test_propagate_down failed... Not all ASes have refrence when they should.." << std::endl;
        return false;
    }
    
    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 290 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 89 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 89) {
        std::cerr << "Propagated priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test propagating down with automatic multihomed mode (default behaviour) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Starting propagation at 1, everyone but 3 and 6 should see the announcement.
 */
bool test_propagate_down2() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(1)->second->process_announcement(ann, true);
    e.propagate_down();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 1 &&
        e.graph->ases->find(2)->second->all_anns->size() == 1 &&
        e.graph->ases->find(3)->second->all_anns->size() == 0 &&
        e.graph->ases->find(4)->second->all_anns->size() == 1 &&
        e.graph->ases->find(5)->second->all_anns->size() == 1 &&
        e.graph->ases->find(6)->second->all_anns->size() == 0)) {
        std::cerr << "test_propagate_down2 failed... Not all ASes have refrence when they should.." << std::endl;
        return false;
    }
    
    if (e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 290 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 89 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 88 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 88) {
        std::cerr << "Propagated priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test multihomed propagating down in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *     1
 *    /|
 *   4 2
 *    \|   
 *     3--5
 *
 *  Starting propagation at 1, everyone but 5 should see the announcement.
 */
bool test_propagate_down_multihomed_standard() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 2, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 4, AS_REL_PROVIDER);
    e.graph->add_relationship(4, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 5, AS_REL_PEER);
    e.graph->add_relationship(5, 3, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(1)->second->process_announcement(ann, true);
    e.propagate_down();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 1 &&
        e.graph->ases->find(2)->second->all_anns->size() == 1 &&
        e.graph->ases->find(3)->second->all_anns->size() == 1 &&
        e.graph->ases->find(4)->second->all_anns->size() == 1 &&
        e.graph->ases->find(5)->second->all_anns->size() == 0)){
        
        std::cerr << "test_propagate_down_multihomed_standard failed... Not all ASes have refrence when they should.." << std::endl;
        return false;
    }
    
    if (e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 290 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 89 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 89 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 88) {
        std::cerr << "Propagted priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test send_all_announcements with automatic multihomed mode (default behaviour) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *  1
 *  |
 *  2 3
 *   \|
 *    4
 *
 *  Sending announcements from 4, everyone but 1 should see the announcement.
 */
bool test_send_all_announcements() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 4, AS_REL_CUSTOMER);

    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    delete as_path;

    // Check to providers
    e.send_all_announcements(4, true, false, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(4, false, true, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(4, false, false, true);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }

    // Process announcements to get the correct announcement priority
    e.graph->ases->find(2)->second->process_announcements(true);
    e.graph->ases->find(3)->second->process_announcements(true);
    e.graph->ases->find(4)->second->process_announcements(true);

    // Check priority calculation
    if (e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 299) {
        std::cerr << "Send all announcement priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test send_all_announcements with automatic multihomed mode (default behaviour) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *  1
 *  |
 *  2 3
 *   \|
 *    4
 *
 *  Sending announcements from 4 with existing announcements at 2 and 3 (same prefix as 4 and origin asn = 4), no one should see the announcement.
 */
bool test_send_all_announcements2() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 4, AS_REL_CUSTOMER);

    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(2);
    as_path->push_back(4);
    std::vector<uint32_t> *as_path2 = new std::vector<uint32_t>();
    as_path2->push_back(3);
    as_path2->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    e.give_ann_to_as_path(as_path2, p);
    delete as_path;
    delete as_path2;

    // Check to providers
    e.send_all_announcements(4, true, false, false);

    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(4, false, true, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(4, false, false, true);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }

    // Process announcements to get the correct announcement priority
    e.graph->ases->find(2)->second->process_announcements(true);
    e.graph->ases->find(3)->second->process_announcements(true);
    e.graph->ases->find(4)->second->process_announcements(true);

    // Check priority calculation
    if (e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 299) {
        std::cerr << "Send all announcement priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test send_all_announcements with automatic multihomed mode (default behaviour) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *  1
 *  |
 *  2 3
 *   \|
 *    4
 *
 *  Sending announcements from 4 with existing announcements at 2 (same prefix as 4 and origin asn = 4), no one should see the announcement.
 */
bool test_send_all_announcements3() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 4, AS_REL_CUSTOMER);

    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(2);
    as_path->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    delete as_path;

    // Check to providers
    e.send_all_announcements(4, true, false, false);

    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(4, false, true, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(4, false, false, true);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }

    // Process announcements to get the correct announcement priority
    e.graph->ases->find(2)->second->process_announcements(true);
    e.graph->ases->find(3)->second->process_announcements(true);
    e.graph->ases->find(4)->second->process_announcements(true);

    // Check priority calculation
    if (e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299) {
        std::cerr << "Send all announcement priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test send_all_announcements without multihomed support in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2---3
 *   /|    \
 *  4 5--6  7
 *
 *  Starting propagation at 2, only 6 and 7 should not see the announcement.
 */
bool test_send_all_announcements_no_multihomed() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 0, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(7, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 7, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(2);
    as_path->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    delete as_path;

    // Check to providers
    e.send_all_announcements(2, true, false, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->all_anns->size() == 0 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(2, false, true, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(2, false, false, true);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }

    // Process announcements to get the correct announcement priority
    e.graph->ases->find(1)->second->process_announcements(true);
    e.graph->ases->find(2)->second->process_announcements(true);
    e.graph->ases->find(3)->second->process_announcements(true);
    e.graph->ases->find(5)->second->process_announcements(true);

    // Check priority calculation
    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 298 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 98) {
        std::cerr << "Send all announcement priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test send_all_announcements with multihomed detection enabled in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2---3
 *   /|    \
 *  4 5--6  7
 * 
 *  Starting propagation at 2, only 6 and 7 should not see the announcement.
 */
bool test_send_all_announcements_multihomed_standard1() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 2, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(7, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 7, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(2);
    as_path->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    delete as_path;

    // Check to providers
    e.send_all_announcements(2, true, false, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->all_anns->size() == 0 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(2, false, true, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->all_anns->size() == 0 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(2, false, false, true);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }
    

    // Process announcements to get the correct announcement priority
    e.graph->ases->find(1)->second->process_announcements(true);
    e.graph->ases->find(2)->second->process_announcements(true);
    e.graph->ases->find(3)->second->process_announcements(true);
    e.graph->ases->find(5)->second->process_announcements(true);

    // Check priority calculation
    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 298 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 98) {
        std::cerr << "send_all_announcements_multihomed_standard1 priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test send_all_announcements with multihomed detection enabled in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2---3
 *   /|    \
 *  4 5--6  7
 * 
 *  Starting propagation at 5, only 5 should see the announcement.
 */
bool test_send_all_announcements_multihomed_standard2() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 2, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(7, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 7, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(5);
    //as_path->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    delete as_path;

    // Check to providers
    e.send_all_announcements(5, true, false, false);
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
          e.graph->ases->find(2)->second->all_anns->size() == 0 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 0 &&
          e.graph->ases->find(5)->second->all_anns->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(5, false, true, false);
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
          e.graph->ases->find(2)->second->all_anns->size() == 0 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 0 &&
          e.graph->ases->find(5)->second->all_anns->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(5, false, false, true);
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
          e.graph->ases->find(2)->second->all_anns->size() == 0 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 0 &&
          e.graph->ases->find(5)->second->all_anns->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }

    // Check priority calculation
    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400) {
        std::cerr << "send_all_announcements_multihomed_standard2 priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test send_all_announcements with multihomed detection enabled (propagate to peers) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2---3
 *   /|    \
 *  4 5--6  7
 * 
 *  Starting propagation at 5, only 6 and 7 should not see the announcement.
 */
bool test_send_all_announcements_multihomed_peer_mode1() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 3, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(7, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 7, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(2);
    as_path->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    delete as_path;

    // Check to providers
    e.send_all_announcements(2, true, false, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->all_anns->size() == 0 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(2, false, true, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(2, false, false, true);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->all_anns->size() == 1 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->all_anns->size() == 1 &&
          e.graph->ases->find(5)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }
    
    // Process announcements to get the correct announcement priority
    e.graph->ases->find(1)->second->process_announcements(true);
    e.graph->ases->find(2)->second->process_announcements(true);
    e.graph->ases->find(3)->second->process_announcements(true);
    e.graph->ases->find(5)->second->process_announcements(true);

    // Check priority calculation
    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 298 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198 ||
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 98) {
        std::cerr << "send_all_announcements_multihomed_peer_mode1 priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test send_all_announcements with multihomed detection enabled (propagate to peers) in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2---3
 *   /|    \
 *  4 5--6  7
 * 
 *  Starting propagation at 5, only 5 and 6 should see the announcement.
 */
bool test_send_all_announcements_multihomed_peer_mode2() {
    Extrapolator e = Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, 
                                            DEFAULT_QUERIER_CONFIG_SECTION, DEFAULT_ITERATION_SIZE, -1, 3, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(7, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 7, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(5);
    //as_path->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    delete as_path;

    // Check to providers
    e.send_all_announcements(5, true, false, false);
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
          e.graph->ases->find(2)->second->all_anns->size() == 0 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 0 &&
          e.graph->ases->find(5)->second->all_anns->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(5, false, true, false);
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
          e.graph->ases->find(2)->second->all_anns->size() == 0 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 0 &&
          e.graph->ases->find(5)->second->all_anns->size() == 1 &&
          e.graph->ases->find(6)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(5, false, false, true);
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
          e.graph->ases->find(2)->second->all_anns->size() == 0 &&
          e.graph->ases->find(3)->second->all_anns->size() == 0 &&
          e.graph->ases->find(4)->second->all_anns->size() == 0 &&
          e.graph->ases->find(5)->second->all_anns->size() == 1 &&
          e.graph->ases->find(6)->second->all_anns->size() == 0 &&
          e.graph->ases->find(7)->second->all_anns->size() == 0)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }
    
    // Process announcements to get the correct announcement priority
    e.graph->ases->find(5)->second->process_announcements(true);
    e.graph->ases->find(6)->second->process_announcements(true);

    // Check priority calculation
    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(6)->second->all_anns->find(p)->second.priority != 199) {
        std::cerr << "send_all_announcements_multihomed_peer_mode2 priority calculation failed." << std::endl;
        return false;
    }

    return true;
}

/** Test saving results in the following test graph (same as the propagate_down test graph).
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Starting propagation at 1, everyone should see the announcement.
 *
 *  This test is currently requires manual verification that the results in the database are correct.
 */
bool test_save_results_parallel() {
    Extrapolator e = Extrapolator(false, true, false, false, "ignored", "test_extrapolation_results", "unused", "unused", "unused", "bgp", 10000, -1, 0, 
    DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(1)->second->process_announcement(ann, true);
    e.propagate_down();
    // No need to propagate up, the announcement started at the top

    e.querier->clear_results_from_db();
    e.querier->create_results_tbl();
    e.save_results(0);

    // This test requires manual verification of correctness.

    return true;
}

/** Test saving results for a single AS in the following test graph (same as
 *  the propagate_down test graph).  Horizontal lines are peer relationships,
 *  vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Starting propagation at 1, everyone should see the announcement.
 *
 *  This test is currently requires manual verification that the results in the database are correct.
 */
bool test_save_results_at_asn() {
    Extrapolator e = Extrapolator(false, false, false, true, "ignored", "unused", "unused", "unused", "test_extrapolation_single_results", "bgp", 10000, -1, 0, 
    DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(1)->second->process_announcement(ann, true);
    e.propagate_down();
    // No need to propagate up, the announcement started at the top

    e.querier->clear_full_path_from_db();
    e.querier->create_full_path_results_tbl();
    e.save_results_at_asn(5);

    // This test requires manual verification of correctness.

    return true;
}

/** 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Test prepending on path vector [3, 3, 2, 5]. This shouldn't change the priority since prepending is in the back of the path. 
 */
bool test_prepending_priority_back() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198) {
        std::cerr << "prepending_priority_back failed." << std::endl;
        return false;
    }

    return true;
}

/** 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Test prepending on path vector [3, 2, 2, 5]. This should reduce the priority at 3. 
 */
bool test_prepending_priority_middle() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(2);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 197) {
        std::cerr << "prepending_priority_middle failed." << std::endl;
        return false;
    }

    return true;
}

/** 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Test prepending on path vector [3, 2, 5, 5]. This should reduce the priority at 3 and 5. 
 */
bool test_prepending_priority_beginning() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 298 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 197) {
        std::cerr << "prepending_priority_end failed." << std::endl;
        return false;
    }

    return true;
}

/** 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Test prepending on path vector [3, 3, 2, 5] with an existing announcement at those ASes. This shouldn't change the priority since prepending is in the back of the path.  
 */
bool test_prepending_priority_back_existing_ann() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);

    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(3);
    as_path_b->push_back(3);
    as_path_b->push_back(2);
    as_path_b->push_back(5);
    e.give_ann_to_as_path(as_path_b, p, 2);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198) {
        std::cerr << "prepending_priority_back_existing_ann failed." << std::endl;
        return false;
    }

    return true;
}

/** 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Test prepending on path vector [3, 2, 2, 5] with an existing announcement at those ASes. This shouldn't change the priority since the existing announcement has higher priority.  
 */
bool test_prepending_priority_middle_existing_ann() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);

    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(3);
    as_path_b->push_back(2);
    as_path_b->push_back(2);
    as_path_b->push_back(5);
    e.give_ann_to_as_path(as_path_b, p, 2);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198) {
        std::cerr << "prepending_priority_middle_existing_ann failed." << std::endl;
        return false;
    }

    return true;
}

/** 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Test prepending on path vector [3, 2, 5, 5] with an existing announcement at those ASes. This shouldn't change the priority since the existing announcement has higher priority.  
 */
bool test_prepending_priority_beginning_existing_ann() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);
    
    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(3);
    as_path_b->push_back(2);
    as_path_b->push_back(5);
    as_path_b->push_back(5);
    e.give_ann_to_as_path(as_path_b, p, 2);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198) {
        std::cerr << "prepending_priority_beginning_existing_ann failed." << std::endl;
        return false;
    }
    
    return true;
}

/** 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Test prepending on path vector [3, 2, 5, 5] with an existing announcement at those ASes. This should change the priority since the existing announcement has lower priority.  
 */
bool test_prepending_priority_beginning_existing_ann2() {
    Extrapolator e = Extrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);
    
    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 298 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 197) {
        std::cerr << "prepending_priority_beginning_existing_ann2 failed." << std::endl;
        return false;
    }

    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(3);
    as_path_b->push_back(2);
    as_path_b->push_back(5);
    e.give_ann_to_as_path(as_path_b, p, 2);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 400 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198) {
        std::cerr << "prepending_priority_beginning_existing_ann2 failed." << std::endl;
        return false;
    }
    
    return true;
}

// Create an announcements table and insert two announcements with different prefixes
bool test_extrapolate_blocks_buildup() {
    try {
        std::string announcements_table = TEST_ANNOUNCEMENTS_TABLE;

        SQLQuerier *querier = new SQLQuerier("ignored", "ignored", "ignored", "ignored", -1, "bgp");

        std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + announcements_table + " (\
        prefix cidr, as_path bigint[], origin bigint, time bigint); GRANT ALL ON TABLE " + announcements_table + " TO bgp_user;");
        querier->execute(sql, false);

        sql = std::string("TRUNCATE " + announcements_table + ";");
        querier->execute(sql, true);

        sql = std::string("INSERT INTO " + announcements_table + " VALUES ('137.99.0.0/16', '{1}', 1, 0), ('137.98.0.0/16', '{5}', 5, 0);");
        querier->execute(sql, true);

        delete querier;
    } catch (const std::exception &e) {
        std::cerr << "Extrapolate blocks buildup failed" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

// Drop an announcements table created in the buildup function
bool test_extrapolate_blocks_teardown() {
    try {
        std::string announcements_table = TEST_ANNOUNCEMENTS_TABLE;
        std::string results_table = TEST_RESULTS_TABLE;

        SQLQuerier *querier = new SQLQuerier("ignored", "ignored", "ignored", "ignored", -1, "bgp");

        std::string sql = std::string("DROP TABLE IF EXISTS " + announcements_table + ", " + results_table + ";");
        querier->execute(sql, false);

        delete querier;
    } catch (const std::exception &e) {
        std::cerr << "Extrapolate blocks teardown failed" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

/** Test extrapolate blocks in the following test graph (same as the propagate_down test graph).
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Extrapolate two announcements with different prefixes (from AS 1 and AS 5).
 */
bool test_extrapolate_blocks() {
    std::string announcements_table = TEST_ANNOUNCEMENTS_TABLE;
    std::string results_table = TEST_RESULTS_TABLE;

    Extrapolator e = Extrapolator(false, false, false, announcements_table, results_table, "unused", "unused", "bgp", 10000, -1, 0);
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    
    std::vector<Prefix<>*> *subnet_blocks = new std::vector<Prefix<>*>;
    
    Prefix<>* p1 = new Prefix<>("137.99.0.0", "255.255.0.0");
    subnet_blocks->push_back(p1);

    Prefix<>* p2 = new Prefix<>("137.98.0.0", "255.255.0.0");
    subnet_blocks->push_back(p2);

    e.querier->clear_results_from_db();
    e.querier->create_results_tbl();
    
    uint32_t announcement_count = 0; 
    int iteration = 0;

    e.extrapolate_blocks(announcement_count, iteration, true, subnet_blocks);

    // Vector that contains correct extrapolation results
    // Format: (asn prefix origin received_from_asn time)
    std::vector<std::string> true_results {
        "1 137.99.0.0/16 1 1 0 ",
        "2 137.99.0.0/16 1 1 0 ",
        "4 137.99.0.0/16 1 2 0 ",
        "5 137.99.0.0/16 1 2 0 ",
        "1 137.98.0.0/16 5 2 0 ",
        "2 137.98.0.0/16 5 5 0 ",
        "3 137.98.0.0/16 5 2 0 ",
        "4 137.98.0.0/16 5 2 0 ",
        "5 137.98.0.0/16 5 5 0 ",
        "6 137.98.0.0/16 5 5 0 " 
    };

    // Get extrapolation results
    pqxx::result r = e.querier->select_from_table(results_table);

    // If the number of rows is different from true_results, we already know that something is not right
    if (r.size() != true_results.size()) {
        std::cerr << "Extrapolate blocks failed. Results are incorrect" << std::endl;
        return false;
    }

    // Convert every row in results to a string separated by spaces (same format as true_results)
    // Make sure the results match those in true_results
    for (auto const &row: r) {
        std::string rowStr = "";
        for (auto const &field: row) { 
            rowStr = rowStr + field.c_str() + " ";
        }
        std::vector<std::string>::iterator it = std::find(true_results.begin(), true_results.end(), rowStr);
        if (it != true_results.end()) {
            true_results.erase(it);
        } else {
            std::cerr << "Extrapolate blocks failed. Results are incorrect" << std::endl;
            return false;
        }
    }

    return true;
}