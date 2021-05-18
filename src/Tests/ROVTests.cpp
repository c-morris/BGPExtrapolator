#include "Tests/Tests.h"
#include "Extrapolators/ROVExtrapolator.h"

/** ROVExtrapolator tests, copied mostly from ROVppTests.cpp 
 */


bool test_rov_constructor() {
    ROVExtrapolator e = ROVExtrapolator();
    if (e.graph == NULL) { return false; }
    return true;
}

/** Test is_from_attacker which should return false if the origin of the announcement is an
 *  attacker. 
 *
 * @return True if successful, otherwise false
 */
bool test_rov_is_from_attacker() {
    ROVASGraph graph = ROVASGraph();
    graph.add_relationship(1, 2, AS_REL_PROVIDER);
    graph.add_relationship(2, 1, AS_REL_CUSTOMER);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    ROVAnnouncement ann = ROVAnnouncement(13796, p, 22742);

    // No attackers
    if (graph.ases->find(1)->second->is_from_attacker(ann) == true) {
        return false;
    }

    // Origin is not an attacker
    graph.add_attacker(666);
    if (graph.ases->find(1)->second->is_from_attacker(ann) == true) {
        return false;
    }

    // Origin is an attacker
    graph.add_attacker(13796);
    if (graph.ases->find(1)->second->is_from_attacker(ann) == false) {
        return false;
    }
    return true;
}

/** Test is_attacker which should return true if the AS is an attacker. 
 *
 * @return True if successful, otherwise false
 */
bool test_rov_is_attacker() {
    std::set<uint32_t> attackers;
    attackers.insert(1);
    ROVAS as = ROVAS(1, &attackers);

    return as.is_attacker();
}

/** Test directly adding an announcement to an AS.
 *
 * @return true if successful.
 */
bool test_rov_process_announcement(){
    ROVAnnouncement ann = ROVAnnouncement(13796, Prefix<>(0x89630000, 0xFFFF0000, 0, 0), 22742);
    ROVASGraph graph = ROVASGraph();
    ROVAS as = *graph.createNew(1);
    // this function should make a copy of the announcement
    // if it does not, it is incorrect
    as.process_announcement(ann);
    Prefix<> old_prefix = ann.prefix;
    ann.prefix.addr = 0x321C9F00;
    ann.prefix.netmask = 0xFFFFFF00;
    Prefix<> new_prefix = ann.prefix;
    as.process_announcement(ann);
    if (new_prefix != as.all_anns->find(ann.prefix)->prefix ||
        old_prefix != as.all_anns->find(old_prefix)->prefix) {
        return false;
    }

    // When an AS adopts ROV, an announcement should be processed when it's not from an attacker
    graph.add_attacker(535);
    as.set_rov_adoption(true);
    as.all_anns->clear();
    as.process_announcement(ann);
    if (as.all_anns->size() != 1) {
        std::cerr << "Failed process announcement, adopts ROV, good announcement" << std::endl;
        return false;
    }

    // When an AS doesn't adopt ROV, an announcement should be processed at all times
    // Set ann's origin to an attacker
    ann.origin = 535;
    as.set_rov_adoption(false);
    as.all_anns->clear();
    as.process_announcement(ann);
    if (as.all_anns->size() != 1) {
        std::cerr << "Failed process announcement, doesn't adopt ROV" << std::endl;
        return false;
    }

    // When an AS is an attacker, an announcement should be processed at all times
    // Even if an AS adopts ROV
    as.asn = 535;
    as.set_rov_adoption(true);
    as.all_anns->clear();
    as.process_announcement(ann);
    if (as.all_anns->size() != 1) {
        std::cerr << "Failed process announcement, attacker" << std::endl;
        return false;
    }

    // Don't need to check priority or new best annoucement here because it uses parent's function
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
bool test_rov_give_ann_to_as_path() {
    ROVExtrapolator e = ROVExtrapolator();
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

    // Test that monitor annoucements were received
    if(!(e.graph->ases->find(2)->second->all_anns->find(p)->from_monitor &&
         e.graph->ases->find(3)->second->all_anns->find(p)->from_monitor &&
         e.graph->ases->find(5)->second->all_anns->find(p)->from_monitor)) {
        std::cerr << "Monitor flag failed." << std::endl;
        delete as_path;
        return false;
    }

    // Test announcement priority calculation
    if (e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) 253 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) 254 << 8) ||
        e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8)) {
        std::cerr << "Priority calculation failed." << std::endl;
        delete as_path;
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
        delete as_path;
        return false;
    }

    // Test timestamp tie breaking
    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(1);
    as_path_b->push_back(2);
    as_path_b->push_back(4);
    as_path_b->push_back(4);
    e.give_ann_to_as_path(as_path_b, p, 1);

    if (e.graph->ases->find(2)->second->all_anns->find(p)->tstamp != 1) {
        delete as_path;
        delete as_path_b;
        return false;
    }
    
    // Test prepending calculation
    if (e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) 253 << 8)) {
        std::cout << e.graph->ases->find(2)->second->all_anns->find(p)->priority << std::endl;
        delete as_path;
        delete as_path_b;
        return false;
    }

    delete as_path;
    delete as_path_b;
    return true;
}

/** Test seeding the graph with announcements from monitors with a ROA invalid path. 
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
bool test_rov_give_ann_to_as_path_invalid() {
    ROVExtrapolator e = ROVExtrapolator();
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

    // Set ROV adoption manually (normally set in ROVASGraph::create_graph_from_db)
    e.graph->ases->find(3)->second->set_rov_adoption(true);
    e.graph->ases->find(2)->second->set_rov_adoption(true);
    e.graph->ases->find(5)->second->set_rov_adoption(true);

    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(3);
    as_path->push_back(2);
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    e.give_ann_to_as_path(as_path, p, 2, 2);

    // Test that only path received the announcement
    if (!(e.graph->ases->find(1)->second->all_anns->size() == 0 &&
        e.graph->ases->find(2)->second->all_anns->size() == 0 &&
        e.graph->ases->find(3)->second->all_anns->size() == 0 &&
        e.graph->ases->find(4)->second->all_anns->size() == 0 &&
        e.graph->ases->find(5)->second->all_anns->size() == 1 &&
        e.graph->ases->find(6)->second->all_anns->size() == 0)) {
        std::cerr << "MRT overseeding check failed." << std::endl;
        delete as_path;
        return false;
    }

    // Test that monitor annoucements were received
    if (!e.graph->ases->find(5)->second->all_anns->find(p)->from_monitor) {
        std::cerr << "Monitor flag failed." << std::endl;
        delete as_path;
        return false;
    }

    // Test announcement priority calculation
    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8)) {
        std::cerr << "Priority calculation failed." << std::endl;
        delete as_path;
        return false;
    }

    delete as_path;
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
bool test_rov_send_all_announcements() {
    ROVExtrapolator e = ROVExtrapolator();
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
    e.give_ann_to_as_path(as_path, p, 0);
    delete as_path;

    // Check to providers
    e.send_all_announcements(2, true, false, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(4)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(5)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(6)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(7)->second->incoming_announcements->size() == 0)) {
        std::cerr << "Error sending to providers" << std::endl;
        return false;
    }
    
    // Check to peers
    e.send_all_announcements(2, false, true, false);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(5)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(6)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(7)->second->incoming_announcements->size() == 0)) {
        std::cerr << "Error sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(2, false, false, true);
    if (!(e.graph->ases->find(1)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(2)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(3)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(4)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(5)->second->incoming_announcements->size() == 1 &&
          e.graph->ases->find(6)->second->incoming_announcements->size() == 0 &&
          e.graph->ases->find(7)->second->incoming_announcements->size() == 0)) {
        std::cerr << "Error sending to customers" << std::endl;
        return false;
    }

    // Process announcements to get the correct announcement priority
    e.graph->ases->find(1)->second->process_announcements(true);
    e.graph->ases->find(2)->second->process_announcements(true);
    e.graph->ases->find(3)->second->process_announcements(true);
    e.graph->ases->find(4)->second->process_announcements(true);
    e.graph->ases->find(5)->second->process_announcements(true);

    // Check priority calculation
    if (e.graph->ases->find(4)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 1) << 8) ||
        e.graph->ases->find(1)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 2) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 2) << 8) ||
        e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) (255 - 2) << 8)) {
        std::cerr << "Send all announcements priority calculation failed." << std::endl;
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
 *  Test prepending on path vector [3, 3, 2, 5]. This shouldn't change the priority since prepending is in the back of the path. 
 */
bool test_rov_prepending_priority_back() {
    ROVExtrapolator e = ROVExtrapolator();
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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    e.give_ann_to_as_path(as_path, p, 2, 0);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 1) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 2) << 8)) {
        std::cerr << "rov_prepending_priority_back failed." << std::endl;
        delete as_path;
        return false;
    }
    delete as_path;
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
bool test_rov_prepending_priority_middle() {
    ROVExtrapolator e = ROVExtrapolator();
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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    e.give_ann_to_as_path(as_path, p, 2, 0);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 1) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 3) << 8)) {
        std::cerr << "rov_prepending_priority_middle failed." << std::endl;
        delete as_path;
        return false;
    }
    delete as_path;
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
bool test_rov_prepending_priority_beginning() {
    ROVExtrapolator e = ROVExtrapolator();
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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    e.give_ann_to_as_path(as_path, p, 2, 0);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 2) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 3) << 8)) {
        std::cerr << "rov_prepending_priority_end failed." << std::endl;
        delete as_path;
        return false;
    }
    delete as_path;
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
bool test_rov_prepending_priority_back_existing_ann() {
    ROVExtrapolator e = ROVExtrapolator();
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
    e.give_ann_to_as_path(as_path, p, 2, 0);

    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(3);
    as_path_b->push_back(3);
    as_path_b->push_back(2);
    as_path_b->push_back(5);
    e.give_ann_to_as_path(as_path_b, p, 2, 0);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 1) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 2) << 8)) {
        std::cerr << "rov_prepending_priority_back_existing_ann failed." << std::endl;
        delete as_path;
        delete as_path_b;
        return false;
    }
    delete as_path;
    delete as_path_b;
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
bool test_rov_prepending_priority_middle_existing_ann() {
    ROVExtrapolator e = ROVExtrapolator();
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
    e.give_ann_to_as_path(as_path, p, 2, 0);
    
    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(3);
    as_path_b->push_back(2);
    as_path_b->push_back(2);
    as_path_b->push_back(5);
    e.give_ann_to_as_path(as_path_b, p, 2, 0);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 1) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 2) << 8)) {
        std::cerr << "rov_prepending_priority_middle_existing_ann failed." << std::endl;
        delete as_path;
        delete as_path_b;
        return false;
    }
    delete as_path;
    delete as_path_b;
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
bool test_rov_prepending_priority_beginning_existing_ann() {
    ROVExtrapolator e = ROVExtrapolator();
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
    e.give_ann_to_as_path(as_path, p, 2, 0);

    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(3);
    as_path_b->push_back(2);
    as_path_b->push_back(5);
    as_path_b->push_back(5);
    e.give_ann_to_as_path(as_path_b, p, 2, 0);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 1) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 2) << 8)) {
        std::cerr << "rov_prepending_priority_beginning_existing_ann failed." << std::endl;
        delete as_path;
        delete as_path_b;
        return false;
    }
    delete as_path;
    delete as_path_b;
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
bool test_rov_prepending_priority_beginning_existing_ann2() {
    ROVExtrapolator e = ROVExtrapolator();
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
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    e.give_ann_to_as_path(as_path, p, 2, 0);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 2) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 3) << 8)) {
        std::cerr << "rov_prepending_priority_beginning_existing_ann2 failed." << std::endl;
        delete as_path;
        return false;
    }

    std::vector<uint32_t> *as_path_b = new std::vector<uint32_t>();
    as_path_b->push_back(3);
    as_path_b->push_back(2);
    as_path_b->push_back(5);
    e.give_ann_to_as_path(as_path_b, p, 2, 0);

    if (e.graph->ases->find(5)->second->all_anns->find(p)->priority != ((uint64_t) 3 << 24) + ((uint64_t) 255 << 8) ||
        e.graph->ases->find(2)->second->all_anns->find(p)->priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 1) << 8) ||
        e.graph->ases->find(3)->second->all_anns->find(p)->priority != ((uint64_t) 1 << 24) + ((uint64_t) (255 - 2) << 8)) {
        std::cerr << "rov_prepending_priority_beginning_existing_ann2 failed." << std::endl;
        delete as_path;
        delete as_path_b;
        return false;
    }
    delete as_path;
    delete as_path_b;
    return true;
}