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

#include "ROVppExtrapolator.h"

/** ROVppExtrapolator tests, copied from ExtrapolatorTest.cpp 
 */

bool test_ROVppExtrapolator_constructor() {
    ROVppExtrapolator e = ROVppExtrapolator();
    if (e.graph == NULL) { return false; }
    return true;
}

/** Test the loop detection in input MRT AS paths.
 */
bool test_rovpp_find_loop() {
    ROVppExtrapolator e = ROVppExtrapolator();
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
bool test_rovpp_give_ann_to_as_path() {
    ROVppExtrapolator e = ROVppExtrapolator();
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
    if (e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 198 &&
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 &&
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
bool test_rovpp_propagate_up() {
    ROVppExtrapolator e = ROVppExtrapolator();
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
    e.graph->ases->find(5)->second->process_announcement(ann);
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
    
    // Check propagation priority calculation
    if (e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 290 &&
        e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 289 &&
        e.graph->ases->find(6)->second->all_anns->find(p)->second.priority != 189 &&
        e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 288 &&
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 188) {
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
bool test_rovpp_propagate_down() {
    ROVppExtrapolator e = ROVppExtrapolator();
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
    e.graph->ases->find(2)->second->process_announcement(ann);
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
    
    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 290 &&
        e.graph->ases->find(4)->second->all_anns->find(p)->second.priority != 89 &&
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 89) {
        std::cerr << "Propagted priority calculation failed." << std::endl;
        return false;
    }
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
bool test_rovpp_send_all_announcements() {
    ROVppExtrapolator e = ROVppExtrapolator();
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
    
    // Check priority calculation
    if (e.graph->ases->find(2)->second->all_anns->find(p)->second.priority != 299 &&
        e.graph->ases->find(1)->second->all_anns->find(p)->second.priority != 289 &&
        e.graph->ases->find(3)->second->all_anns->find(p)->second.priority != 189 &&
        e.graph->ases->find(5)->second->all_anns->find(p)->second.priority != 89) {
        std::cerr << "Send all announcement priority calculation failed." << std::endl;
        return false;
    }

    return true;
}
