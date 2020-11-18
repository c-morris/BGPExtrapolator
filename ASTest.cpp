#include <iostream>
#include "AS.h"
#include "Announcement.h"

/** Unit tests for AS.cpp
 */

/** Test deterministic randomness within the AS scope.
 *
 * @return True if successful, otherwise false
 */
bool test_get_random(){
    // Check randomness
    AS *as_a = new AS(832);
    AS *as_b = new AS(832);
    bool ran_a_1 = as_a->get_random();
    bool ran_a_2 = as_a->get_random();
    bool ran_a_3 = as_a->get_random();
    bool ran_b_1 = as_b->get_random();
    bool ran_b_2 = as_b->get_random();
    bool ran_b_3 = as_b->get_random();
    delete as_a;
    delete as_b;
    if (ran_a_1 != ran_b_1 || ran_a_2 != ran_b_2 || ran_a_3 != ran_b_3) {
        std::cerr << ran_a_1 << " != " << ran_b_1 << std::endl;
        std::cerr << ran_a_2 << " != " << ran_b_2 << std::endl;
        std::cerr << ran_a_3 << " != " << ran_b_3 << std::endl;
        std::cerr << "Failed deterministic randomness check." << std::endl;
        return false;
    }
    return true;
}

/** Test adding neighbor AS to the appropriate set based on the relationship.
 *
 * @return True if successful, otherwise false
 */
bool test_add_neighbor(){
    AS as = AS();
    as.add_neighbor(1, AS_REL_PROVIDER);
    as.add_neighbor(2, AS_REL_PEER);
    as.add_neighbor(3, AS_REL_CUSTOMER);
    if (as.providers->find(1) == as.providers->end() ||
        as.peers->find(2) == as.peers->end() ||
        as.customers->find(3) == as.customers->end()) {
        std::cerr << "Failed add neighbor check." << std::endl;
        return false;
    }
    return true;
}

/** Test removing neighbor AS from the appropriate set based on the relationship.
 *
 * @return True if successful, otherwise false
 */
bool test_remove_neighbor(){
    AS as = AS();
    as.add_neighbor(1, AS_REL_PROVIDER);
    as.add_neighbor(2, AS_REL_PEER);
    as.add_neighbor(3, AS_REL_CUSTOMER);
    as.remove_neighbor(1, AS_REL_PROVIDER);
    as.remove_neighbor(2, AS_REL_PEER);
    as.remove_neighbor(3, AS_REL_CUSTOMER);
    if (as.providers->find(1) != as.providers->end() ||
        as.peers->find(2) != as.peers->end() ||
        as.customers->find(3) != as.customers->end()) {
        std::cerr << "Failed remove neighbor check." << std::endl;
        return false;
    }
    return true;
}

/** Test pushing the received announcement to the ribs_in vector. 
 *
 * @return true if successful.
 */
bool test_receive_announcements(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742, 1);
    std::vector<Announcement> vect = std::vector<Announcement>();
    vect.push_back(ann);
    // this function should make a copy of the announcement
    // if it does not, it is incorrect
    Prefix<> old_prefix = ann.prefix;
    ann.prefix.addr = 0x321C9F00;
    ann.prefix.netmask = 0xFFFFFF00;
    Prefix<> new_prefix = ann.prefix;
    vect.push_back(ann);
    AS as = AS();
    as.receive_announcements(vect);
    if (as.ribs_in->size() != 2) { return false; }
    // order really doesn't matter here
    for (Announcement a : *as.ribs_in) {
        if (a.prefix != old_prefix && a.prefix != new_prefix) {
            return false;
        }
    }
    return true;
}

/** Test directly adding an announcement to the loc_rib map.
 *
 * @return true if successful.
 */
bool test_process_announcement(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742, 1);
    // this function should make a copy of the announcement
    // if it does not, it is incorrect
    AS as = AS();
    as.process_announcement(ann, true);
    Prefix<> old_prefix = ann.prefix;
    ann.prefix.addr = 0x321C9F00;
    ann.prefix.netmask = 0xFFFFFF00;
    Prefix<> new_prefix = ann.prefix;
    as.process_announcement(ann, true);
    if (new_prefix != as.loc_rib->find(ann.prefix)->second.prefix ||
        old_prefix != as.loc_rib->find(old_prefix)->second.prefix) {
        return false;
    }

    // Check priority
    Prefix<> p = Prefix<>("1.1.1.0", "255.255.255.0");
    std::vector<uint32_t> x; 
    Announcement a1 = Announcement(111, p.addr, p.netmask, 199, 222, 0, 1, x);
    Announcement a2 = Announcement(111, p.addr, p.netmask, 298, 223, 0, 1, x);
    as.process_announcement(a1, true);
    as.process_announcement(a2, true);
    if (as.loc_rib->find(p)->second.received_from_asn != 223 ||
        as.depref_anns->find(p)->second.received_from_asn != 222) {
        std::cerr << "Failed best path inference priority check." << std::endl;
        return false;
    }    

    // Check new best announcement
    Announcement a3 = Announcement(111, p.addr, p.netmask, 299, 224, 0, 1, x);
    as.process_announcement(a3, true);
    if (as.loc_rib->find(p)->second.received_from_asn != 224 ||
        as.depref_anns->find(p)->second.received_from_asn != 223) {
        std::cerr << "Failed best path priority correction check." << std::endl;
        return false;
    } 
    return true;
}

/** Test the following properties of the best path selection algorithm.
 *
 * 1. Customer route > peer route > provider route
 * 2. Shortest path takes priority
 * 3. Announcements from monitors are never overwritten
 * 4. Announcements for local prefixes are never overwritten
 *
 * Items one, two, and three are all covered by the priority attribute working correctly. 
 * Item three requires the from_monitor attribute to work. 
 */
bool test_process_announcements(){
    Announcement ann1 = Announcement(13796, 0x89630000, 0xFFFF0000, 22742, 1);
    Prefix<> ann1_prefix = ann1.prefix;
    Announcement ann2 = Announcement(13796, 0x321C9F00, 0xFFFFFF00, 22742, 1);
    Prefix<> ann2_prefix = ann2.prefix;
    AS as = AS();
    // build a vector of announcements
    std::vector<Announcement> vect = std::vector<Announcement>();
    ann1.priority = 100;
    ann2.priority = 200;
    ann2.from_monitor = true;
    vect.push_back(ann1);
    vect.push_back(ann2);

    // does it work if loc_rib is empty?
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.loc_rib->find(ann1_prefix)->second.priority != 100) {
        std::cerr << "Failed to add an announcement to an empty map" << std::endl;
        return false;
    }
    
    // higher priority should overwrite lower priority
    vect.clear();
    ann1.priority = 290;
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.loc_rib->find(ann1_prefix)->second.priority != 290) {
        std::cerr << "Higher priority announcements should overwrite lower priority ones." << std::endl;
        return false;
    }
    
    // lower priority should not overwrite higher priority
    vect.clear();
    ann1.priority = 200;
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.loc_rib->find(ann1_prefix)->second.priority != 290) {
        std::cerr << "Lower priority announcements should not overwrite higher priority ones." << std::endl;
        return false;
    }

    // one more test just to be sure
    vect.clear();
    ann1.priority = 299;
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.loc_rib->find(ann1_prefix)->second.priority != 299) {
        std::cerr << "How did you manage to fail here?" << std::endl;
        return false;
    }

    // make sure ann2 doesn't get overwritten, ever, even with higher priority
    vect.clear();
    ann2.priority = 300;
    vect.push_back(ann2);
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.loc_rib->find(ann2_prefix)->second.priority != 200) {
        std::cerr << "Announcements from_monitor should not be overwritten." << std::endl;
        return false;
    }

    return true;
}
/** Test clearing all announcements.
 *
 * @return true if successful.
 */
bool test_clear_announcements(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742, 1);
    AS as = AS();
    // if receive_announcement is broken, this test will also be broken
    as.process_announcement(ann, true);
    if (as.loc_rib->size() != 1) {
        return false;
    }
    as.clear_announcements();
    if (as.loc_rib->size() != 0) {
        return false;
    }
    return true;
}

/** Test checking if announcement is already received by an AS.
 *
 * @return true if successful.
 */
bool test_already_received(){
    Announcement ann1 = Announcement(13796, 0x89630000, 0xFFFF0000, 22742, 1);
    Announcement ann2 = Announcement(13796, 0x321C9F00, 0xFFFFFF00, 22742, 1);
    AS as = AS();
    // if receive_announcement is broken, this test will also be broken
    as.process_announcement(ann1, true);
    if (as.already_received(ann1) && !as.already_received(ann2)) {
        return true;
    }
    return false;
}
