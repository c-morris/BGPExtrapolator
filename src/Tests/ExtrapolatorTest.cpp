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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    e.give_ann_to_as_path(as_path, p, 2);

    auto as_2_search = e.graph->ases->find(2)->second->all_anns->find(p);
    auto as_3_search = e.graph->ases->find(3)->second->all_anns->find(p);
    auto as_5_search = e.graph->ases->find(5)->second->all_anns->find(p);

    if(as_2_search == e.graph->ases->find(2)->second->all_anns->end()) {
        std::cerr << "AS 2 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_3_search == e.graph->ases->find(3)->second->all_anns->end()) {
        std::cerr << "AS 3 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_5_search == e.graph->ases->find(5)->second->all_anns->end()) {
        std::cerr << "AS 5 announcement does not exist!" << std::endl;
        return false;
    }

    // Test that monitor annoucements were received
    if(!((*as_2_search).from_monitor &&
         (*as_3_search).from_monitor &&
         (*as_5_search).from_monitor)) {
        std::cerr << "Monitor flag failed." << std::endl;
        return false;
    }
    
    // Test announcement priority calculation
    if ((*as_3_search).priority != 198 &&
        (*as_2_search).priority != 299 &&
        (*as_5_search).priority != 400) {
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

    auto as_2_search_2 = e.graph->ases->find(2)->second->all_anns->find(p);
    if(as_2_search_2 == e.graph->ases->find(2)->second->all_anns->end()) {
        std::cerr << "as_2_search_2 announcement does not exist for tiebraking!" << std::endl;
        return false;
    }

    if ((*as_2_search_2).tstamp != 1) {
        std::cerr << "as_2_search_2 announcement has the wrong timestamp for tiebraking!" << std::endl;
        return false;
    }
    
    // Test prepending calculation
    if ((*as_2_search_2).priority != 298) {
        std::cout << (*as_2_search_2).priority << std::endl;
        return false;
    }

    delete as_path;
    delete as_path_b;
    return true;
}

/** Test propagating up in the following test graph.
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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    
    Announcement ann = Announcement(13796, p, 22742);
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
        std::cerr << "Loop detection failed." << std::endl;
        return false;
    }
    
    auto as_1_search = e.graph->ases->find(1)->second->all_anns->find(p);
    auto as_2_search = e.graph->ases->find(2)->second->all_anns->find(p);
    auto as_3_search = e.graph->ases->find(3)->second->all_anns->find(p);
    auto as_5_search = e.graph->ases->find(5)->second->all_anns->find(p);
    auto as_6_search = e.graph->ases->find(6)->second->all_anns->find(p);

    if(as_1_search == e.graph->ases->find(1)->second->all_anns->end()) {
        std::cerr << "AS 1 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_2_search == e.graph->ases->find(2)->second->all_anns->end()) {
        std::cerr << "AS 2 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_3_search == e.graph->ases->find(3)->second->all_anns->end()) {
        std::cerr << "AS 3 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_5_search == e.graph->ases->find(5)->second->all_anns->end()) {
        std::cerr << "AS 5 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_6_search == e.graph->ases->find(6)->second->all_anns->end()) {
        std::cerr << "AS 6 announcement does not exist!" << std::endl;
        return false;
    }

    // Check propagation priority calculation
    if ((*as_5_search).priority != 290 &&
        (*as_2_search).priority != 289 &&
        (*as_6_search).priority != 189 &&
        (*as_1_search).priority != 288 &&
        (*as_3_search).priority != 188) {
        std::cerr << "Propagted priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test propagating down in the following test graph.
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
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Announcement ann = Announcement(13796, p, 22742);
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
        return false;
    }
    
    auto as_2_search = e.graph->ases->find(2)->second->all_anns->find(p);
    auto as_4_search = e.graph->ases->find(4)->second->all_anns->find(p);
    auto as_5_search = e.graph->ases->find(5)->second->all_anns->find(p);

    if(as_2_search == e.graph->ases->find(2)->second->all_anns->end()) {
        std::cerr << "AS 2 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_4_search == e.graph->ases->find(4)->second->all_anns->end()) {
        std::cerr << "AS 4 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_5_search == e.graph->ases->find(5)->second->all_anns->end()) {
        std::cerr << "AS 5 announcement does not exist!" << std::endl;
        return false;
    }

    if ((*as_2_search).priority != 290 &&
        (*as_4_search).priority != 89 &&
        (*as_5_search).priority != 89) {
        std::cerr << "Propagted priority calculation failed." << std::endl;
        return false;
    }
    return true;
}

/** Test propagating down in the following test graph.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 *  Starting propagation at 1, everyone should see the announcement.
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
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Announcement ann = Announcement(13796, p, 22742);
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
        std::cerr << e.graph->ases->find(1)->second->all_anns->size() << std::endl;
        std::cerr << e.graph->ases->find(2)->second->all_anns->size() << std::endl;
        std::cerr << e.graph->ases->find(3)->second->all_anns->size() << std::endl;
        std::cerr << e.graph->ases->find(4)->second->all_anns->size() << std::endl;
        std::cerr << e.graph->ases->find(5)->second->all_anns->size() << std::endl;
        std::cerr << e.graph->ases->find(6)->second->all_anns->size() << std::endl;

        return false;
    }
    
    // if (e.graph->ases->find(2)->second->all_anns->find(p).priority != 290 &&
    //     e.graph->ases->find(4)->second->all_anns->find(p).priority != 89 &&
    //     e.graph->ases->find(5)->second->all_anns->find(p).priority != 89) {
    //     std::cerr << "Propagted priority calculation failed." << std::endl;
    //     return false;
    // }

    return true;
}

/** Test send_all_announcements in the following test graph.
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
bool test_send_all_announcements() {
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

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(2);
    as_path->push_back(4);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
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

    auto as_1_search = e.graph->ases->find(1)->second->all_anns->find(p);
    auto as_2_search = e.graph->ases->find(2)->second->all_anns->find(p);
    auto as_3_search = e.graph->ases->find(3)->second->all_anns->find(p);
    auto as_5_search = e.graph->ases->find(5)->second->all_anns->find(p);

    if(as_1_search == e.graph->ases->find(1)->second->all_anns->end()) {
        std::cerr << "AS 1 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_2_search == e.graph->ases->find(2)->second->all_anns->end()) {
        std::cerr << "AS 2 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_3_search == e.graph->ases->find(3)->second->all_anns->end()) {
        std::cerr << "AS 3 announcement does not exist!" << std::endl;
        return false;
    }

    if(as_5_search == e.graph->ases->find(5)->second->all_anns->end()) {
        std::cerr << "AS 5 announcement does not exist!" << std::endl;
        return false;
    }

    // Check priority calculation
    if ((*as_2_search).priority != 299 &&
        (*as_1_search).priority != 289 &&
        (*as_3_search).priority != 189 &&
        (*as_5_search).priority != 89) {
        std::cerr << "Send all announcement priority calculation failed." << std::endl;
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
 */
bool test_save_results_parallel() {
    Extrapolator e = Extrapolator(false, false, false, "ignored", "test_extrapolation_results", "unused", "unused", "bgp", 10000);
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
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Announcement ann = Announcement(13796, p, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(1)->second->process_announcement(ann, true);
    e.propagate_down();
    // No need to propagate up, the announcement started at the top

    e.querier->clear_results_from_db();
    e.querier->create_results_tbl();
    e.save_results(0);

    return true;
}
