#include <iostream>
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
    
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    ann.from_monitor = true;
    e.graph->ases->find(5)->second->receive_announcement(ann);
    e.propagate_up();
    e.propagate_up();
    std::cout << e.graph->ases->find(1)->second->all_anns->size();
    std::cout << e.graph->ases->find(2)->second->all_anns->size();
    std::cout << e.graph->ases->find(5)->second->all_anns->size();
    std::cout << e.graph->ases->find(6)->second->all_anns->size();
    return true;
}
