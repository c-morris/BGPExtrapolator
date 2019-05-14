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
    ann.prefix.addr = 0x321c9f00;
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
    return true;
}

bool test_already_received(){
    return true;
}


bool test_clear_announcements(){
    return true;
}


bool test_process_announcements(){
    return true;
}
