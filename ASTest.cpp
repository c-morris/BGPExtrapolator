/** Unit tests for AS.h and AS.cpp
 */

#include "AS.h"
#include "Announcement.h"
#include <iostream>

bool test_add_neighbor(){
    AS as = AS();
    as.add_neighbor(1, AS_REL_PROVIDER);
    as.add_neighbor(2, AS_REL_PEER);
    as.add_neighbor(3, AS_REL_CUSTOMER);
    if (as.providers->find(1) == as.providers->end() ||
        as.peers->find(2) == as.peers->end() ||
        as.customers->find(3) == as.customers->end()) {
        return false;
    }
    return true;
}

bool test_receive_announcement(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    // this function should make a copy of the announcement
    // if it does not, it is incorrect
    AS as = AS();
    as.receive_announcement(ann);
    Prefix<> old_prefix = ann.prefix;
    ann.prefix.addr = 0x321C9F00;
    ann.prefix.netmask = 0xFFFFFF00;
    Prefix<> new_prefix = ann.prefix;
    as.receive_announcement(ann);
    if (new_prefix == as.all_anns->find(ann.prefix)->second.prefix &&
        old_prefix == as.all_anns->find(old_prefix)->second.prefix) {
        return true;
    }
    return false;
}

bool test_receive_announcements(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
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
    if (as.incoming_announcements->size() != 2) { return false; }
    // order really doesn't matter here
    for (Announcement a : *as.incoming_announcements) {
        if (a.prefix != old_prefix && a.prefix != new_prefix) {
            return false;
        }
    }
    return true;
}

bool test_already_received(){
    Announcement ann1 = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    Announcement ann2 = Announcement(13796, 0x321C9F00, 0xFFFFFF00, 22742);
    AS as = AS();
    // if receive_announcement is broken, this test will also be broken
    as.receive_announcement(ann1);
    if (as.already_received(ann1) && !as.already_received(ann2)) {
        return true;
    }
    return false;
}


bool test_clear_announcements(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    AS as = AS();
    // if receive_announcement is broken, this test will also be broken
    as.receive_announcement(ann);
    if (as.all_anns->size() != 1) {
        return false;
    }
    as.clear_announcements();
    if (as.all_anns->size() != 0) {
        return false;
    }
    return true;
}


bool test_process_announcements(){
    return true;
}
