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
#include "ASes/AS.h"
#include "Announcements/Announcement.h"

/** Unit tests for AS.cpp
 */

/** Test deterministic randomness within the AS scope.
 *
 * @return True if successful, otherwise false
 */
bool test_get_random(){
    // Check randomness
    AS<> *as_a = new AS<>(832);
    AS<> *as_b = new AS<>(832);
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
    AS<> as = AS<>();
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
    AS<> as = AS<>();
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

/** Test pushing the received announcement to the incoming_announcements vector. 
 *
 * @return true if successful.
 */
bool test_receive_announcements(){
    Announcement<> ann = Announcement<>(13796, 0x89630000, 0xFFFF0000, 22742);
    std::vector<Announcement<>> vect = std::vector<Announcement<>>();
    vect.push_back(ann);
    // this function should make a copy of the announcement
    // if it does not, it is incorrect
    Prefix<> old_prefix = ann.prefix;
    ann.prefix.addr = 0x321C9F00;
    ann.prefix.netmask = 0xFFFFFF00;
    Prefix<> new_prefix = ann.prefix;
    vect.push_back(ann);
    AS<> as = AS<>();
    as.receive_announcements(vect);
    if (as.incoming_announcements->size() != 2) { return false; }
    // order really doesn't matter here
    for (Announcement<> a : *as.incoming_announcements) {
        if (a.prefix != old_prefix && a.prefix != new_prefix) {
            return false;
        }
    }
    return true;
}

/** Test directly adding an announcement to the all_anns map.
 *
 * @return true if successful.
 */
bool test_process_announcement(){
    Announcement<> ann = Announcement<>(13796, 0x89630000, 0xFFFF0000, 22742);
    // this function should make a copy of the announcement
    // if it does not, it is incorrect
    AS<> as = AS<>(0, true);
    as.process_announcement(ann, true);
    Prefix<> old_prefix = ann.prefix;
    ann.prefix.addr = 0x321C9F00;
    ann.prefix.netmask = 0xFFFFFF00;
    Prefix<> new_prefix = ann.prefix;
    as.process_announcement(ann, true);
    if (new_prefix != as.all_anns->find(ann.prefix)->second.prefix ||
        old_prefix != as.all_anns->find(old_prefix)->second.prefix) {
        return false;
    }

    // Check priority
    Prefix<> p = Prefix<>("1.1.1.0", "255.255.255.0");
    
    Priority pr1;
    pr1.path_length = 1;
    pr1.relationship = 1;
    Announcement<> a1 = Announcement<>(111, p.addr, p.netmask, pr1, 222, false);

    Priority pr2;
    pr2.path_length = 2;
    pr2.relationship = 2;
    Announcement<> a2 = Announcement<>(111, p.addr, p.netmask, pr2, 223, false);

    as.process_announcement(a1, true);
    as.process_announcement(a2, true);
    if (as.all_anns->find(p)->second.received_from_asn != 223 ||
        as.depref_anns->find(p)->second.received_from_asn != 222) {
        std::cerr << "Failed best path inference priority check." << std::endl;
        return false;
    }    

    // Check new best announcement
    Priority pr3;
    pr3.path_length = 1;
    pr3.relationship = 2;
    Announcement<> a3 = Announcement<>(111, p.addr, p.netmask, pr3, 224, false);

    as.process_announcement(a3, true);
    if (as.all_anns->find(p)->second.received_from_asn != 224 ||
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
    Announcement<> ann1 = Announcement<>(13796, 0x89630000, 0xFFFF0000, 22742);
    Prefix<> ann1_prefix = ann1.prefix;
    Announcement<> ann2 = Announcement<>(13796, 0x321C9F00, 0xFFFFFF00, 22742);
    Prefix<> ann2_prefix = ann2.prefix;
    AS<> as = AS<>();
    // build a vector of announcements
    std::vector<Announcement<>> vect = std::vector<Announcement<>>();

    Priority pr1;
    pr1.relationship = 1;
    ann1.priority = pr1;

    Priority pr2;
    pr2.relationship = 2;
    ann2.priority = pr2;
    ann2.from_monitor = true;
    vect.push_back(ann1);
    vect.push_back(ann2);

    // does it work if all_anns is empty?
    as.receive_announcements(vect);
    as.process_announcements(true);

    if (as.all_anns->find(ann1_prefix)->second.priority != (uint64_t) 1 << 24) {
        std::cerr << "Failed to add an announcement to an empty map" << std::endl;
        return false;
    }
    
    // higher priority should overwrite lower priority
    vect.clear();
    pr1.relationship = 2;
    pr1.path_length = 10; // a bit less than the max value (255)
    ann1.priority = pr1;
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.all_anns->find(ann1_prefix)->second.priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 10) << 8)) {
        std::cerr << "Higher priority announcements should overwrite lower priority ones." << std::endl;
        return false;
    }
    
    // lower priority should not overwrite higher priority
    vect.clear();
    ann1.priority = pr2; // pr2 == 33,554,432
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.all_anns->find(ann1_prefix)->second.priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 10) << 8)) {
        std::cerr << "Lower priority announcements should not overwrite higher priority ones." << std::endl;
        return false;
    }

    // one more test just to be sure
    vect.clear();
    pr1.path_length = 1;
    ann1.priority = pr1;
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.all_anns->find(ann1_prefix)->second.priority != ((uint64_t) 2 << 24) + ((uint64_t) (255 - 1) << 8)) {
        std::cerr << "How did you manage to fail here?" << std::endl;
        return false;
    }

    // make sure ann2 doesn't get overwritten, ever, even with higher priority
    vect.clear();
    pr2.relationship = 3;
    pr2.path_length = 0;
    ann2.priority = pr2;
    vect.push_back(ann2);
    as.receive_announcements(vect);
    as.process_announcements(true);
    if (as.all_anns->find(ann2_prefix)->second.priority != (uint64_t) 2 << 24) {
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
    Announcement<> ann = Announcement<>(13796, 0x89630000, 0xFFFF0000, 22742);
    AS<> as = AS<>();
    // if receive_announcement is broken, this test will also be broken
    as.process_announcement(ann, true);
    if (as.all_anns->size() != 1) {
        return false;
    }
    as.clear_announcements();
    if (as.all_anns->size() != 0) {
        return false;
    }
    return true;
}

/** Test checking if announcement is already received by an AS.
 *
 * @return true if successful.
 */
bool test_already_received(){
    Announcement<> ann1 = Announcement<>(13796, 0x89630000, 0xFFFF0000, 22742);
    Announcement<> ann2 = Announcement<>(13796, 0x321C9F00, 0xFFFFFF00, 22742);
    AS<> as = AS<>();
    // if receive_announcement is broken, this test will also be broken
    as.process_announcement(ann1, true);
    if (as.already_received(ann1) && !as.already_received(ann2)) {
        return true;
    }
    return false;
}
