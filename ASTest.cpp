/** Unit tests for AS.h and AS.cpp
 */

#include "AS.h"
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


bool test_update_rank(){
    return true;
}


bool test_receive_announcement(){
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
