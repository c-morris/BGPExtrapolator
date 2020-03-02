#include <iostream>
#include <cstdint>
#include <vector>
#include "Extrapolator.h"

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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p, 2);

    // Test that monitor annoucements were received
    if(!(e.graph->ases->find(2)->second->loc_rib->find(p)->second.from_monitor &&
         e.graph->ases->find(3)->second->loc_rib->find(p)->second.from_monitor &&
         e.graph->ases->find(5)->second->loc_rib->find(p)->second.from_monitor)) {
        std::cerr << "Monitor flag failed." << std::endl;
        return false;
    }
    
    // Test announcement priority calculation
    if (e.graph->ases->find(3)->second->loc_rib->find(p)->second.priority != 198 &&
        e.graph->ases->find(2)->second->loc_rib->find(p)->second.priority != 299 &&
        e.graph->ases->find(5)->second->loc_rib->find(p)->second.priority != 400) {
        std::cerr << "Priority calculation failed." << std::endl;
        return false;
    }

    // Test that only path received the announcement
    if (!(e.graph->ases->find(1)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(3)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(4)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(5)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(6)->second->loc_rib->size() == 0)) {
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

    if (e.graph->ases->find(2)->second->loc_rib->find(p)->second.tstamp != 1) {
        return false;
    }
    
    // Test prepending calculation
    if (e.graph->ases->find(2)->second->loc_rib->find(p)->second.priority != 298) {
        std::cout << e.graph->ases->find(2)->second->loc_rib->find(p)->second.priority << std::endl;
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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(5)->second->process_announcement(ann, true);
    e.propagate_up();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(3)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(4)->second->loc_rib->size() == 0 &&
          e.graph->ases->find(5)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(6)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(7)->second->loc_rib->size() == 0)) {
        std::cerr << "Loop detection failed." << std::endl;
        return false;
    }
    
    // Check propagation priority calculation
    if (e.graph->ases->find(5)->second->loc_rib->find(p)->second.priority != 290 &&
        e.graph->ases->find(2)->second->loc_rib->find(p)->second.priority != 289 &&
        e.graph->ases->find(6)->second->loc_rib->find(p)->second.priority != 189 &&
        e.graph->ases->find(1)->second->loc_rib->find(p)->second.priority != 288 &&
        e.graph->ases->find(3)->second->loc_rib->find(p)->second.priority != 188) {
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
    
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);
    ann.from_monitor = true;
    ann.priority = 290;
    e.graph->ases->find(2)->second->process_announcement(ann, true);
    e.propagate_down();
    
    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(3)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(4)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(5)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(6)->second->loc_rib->size() == 0)) {
        return false;
    }
    
    if (e.graph->ases->find(2)->second->loc_rib->find(p)->second.priority != 290 &&
        e.graph->ases->find(4)->second->loc_rib->find(p)->second.priority != 89 &&
        e.graph->ases->find(5)->second->loc_rib->find(p)->second.priority != 89) {
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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path, p);
    delete as_path;

    // Check to providers
    e.send_all_announcements(2, true, false, false);
    if (!(e.graph->ases->find(1)->second->ribs_in->size() == 1 &&
          e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(3)->second->loc_rib->size() == 0 &&
          e.graph->ases->find(4)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(5)->second->loc_rib->size() == 0 &&
          e.graph->ases->find(6)->second->loc_rib->size() == 0 &&
          e.graph->ases->find(7)->second->loc_rib->size() == 0)) {
        std::cerr << "Err sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(2, false, true, false);
    if (!(e.graph->ases->find(1)->second->ribs_in->size() == 1 &&
          e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(3)->second->ribs_in->size() == 1 &&
          e.graph->ases->find(4)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(5)->second->ribs_in->size() == 0 &&
          e.graph->ases->find(6)->second->loc_rib->size() == 0 &&
          e.graph->ases->find(7)->second->loc_rib->size() == 0)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(2, false, false, true);
    if (!(e.graph->ases->find(1)->second->ribs_in->size() == 1 &&
          e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(3)->second->ribs_in->size() == 1 &&
          e.graph->ases->find(4)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(5)->second->ribs_in->size() == 1 &&
          e.graph->ases->find(6)->second->loc_rib->size() == 0 &&
          e.graph->ases->find(7)->second->loc_rib->size() == 0)) {
        std::cerr << "Err sending to customers" << std::endl;
        return false;
    }
    
    // Check priority calculation
    if (e.graph->ases->find(2)->second->loc_rib->find(p)->second.priority != 299 &&
        e.graph->ases->find(1)->second->loc_rib->find(p)->second.priority != 289 &&
        e.graph->ases->find(3)->second->loc_rib->find(p)->second.priority != 189 &&
        e.graph->ases->find(5)->second->loc_rib->find(p)->second.priority != 89) {
        std::cerr << "Send all announcement priority calculation failed." << std::endl;
        return false;
    }

    return true;
}
