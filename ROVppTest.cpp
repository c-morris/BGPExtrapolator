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
#include "ROVppASGraph.h"
#include "ROVppAS.h"
#include "ROVppAnnouncement.h"



/////////////////////////////////////////////////
// Auxilliary functions for the tests
/////////////////////////////////////////////////

/** Creates a two way relationship in the given ASGraph with the given types
 *
 * Provided the two ASNs their corresponding types, they're added to the graph
 * with the given relationship and vice-versa relationship.
 */
void add_two_way_relationship(ASGraph * as_graph, uint32_t asn, uint32_t neighbor_asn, int relation) {
    // Add First Relationship (asn to neighbor_asn)
    as_graph->add_relationship(asn, neighbor_asn, relation);

    // Add Corresponding Relationship (neighbor_asn to asn)
    if (relation == AS_REL_PROVIDER) {
        as_graph->add_relationship(neighbor_asn, asn, AS_REL_CUSTOMER);
    } else if (relation == AS_REL_CUSTOMER) {
        as_graph->add_relationship(neighbor_asn, asn, AS_REL_PROVIDER);
    } else {
        // Add Peer Relationship
        as_graph->add_relationship(neighbor_asn, asn, AS_REL_PEER);
    }
}

/** Tells you whether or not the given ASN has the given prefix in its RIB-OUT
 *
 *
 * @return true if "prefix is found in the given asn RIB-OUT", otherwise false.
 */
bool is_infected(ASGraph * as_graph, uint32_t asn, Prefix<> prefix) {
    auto rib_out =  as_graph->ases->find(asn)->second->loc_rib;
    auto search = rib_out->find(prefix);
    if (search == rib_out->end()) {
      return false;
    } else {
      return true;
    }
}

/** Tells you whether or not the given ASN has the given prefix in its RIB-OUT from vector of ASNs
 *
 *
 * @return true if "prefix is found in the given asn RIB-OUT", otherwise false.
 */
bool check_infected_vector(ASGraph * as_graph, std::vector<uint32_t> asns, Prefix<> prefix) {
    for (uint32_t asn : asns) {
       if (is_infected(as_graph, asn, prefix)) {
         return true;
       }
    }
    return false;
}


//-----------------------------------------------
// Topologies
//-----------------------------------------------

/** Creates a ROVppASGraph object that is the same as figure 1 in the paper.
 * 
 * Distinct ASes: 11, 77, 12, 44, 88, 86, 666, 99
 * Victim: 99
 * Attacker: 666
 *
 * Expected outcomes:
 *  Hijacked:
 *  NotHijacked:
 *  Disconnected:
 *
 * @return ROVppASGraph object setup like figure 1
 */
ROVppASGraph figure_1_graph(uint32_t policy_type) {
    // TODO: function is incomplete needs
    ROVppASGraph graph = ROVppASGraph();
    uint32_t attacker_asn = 666;
    uint32_t victim_asn = 99;
    
    // Set Relationships
    // AS 44
    add_two_way_relationship(&graph, 44, 77, AS_REL_PROVIDER);
    add_two_way_relationship(&graph, 44, 78, AS_REL_PROVIDER);
    add_two_way_relationship(&graph, 44, 99, AS_REL_PROVIDER);
    add_two_way_relationship(&graph, 44, 666, AS_REL_PROVIDER);
    // AS 77
    add_two_way_relationship(&graph, 77, 11, AS_REL_PROVIDER);
    // AS 88
    add_two_way_relationship(&graph, 88, 78, AS_REL_PROVIDER);
    add_two_way_relationship(&graph, 88, 86, AS_REL_PROVIDER);
    // AS 78
    add_two_way_relationship(&graph, 78, 12, AS_REL_PROVIDER);
    // AS 86
    add_two_way_relationship(&graph, 86, 99, AS_REL_PROVIDER);
    
    // Set Attacker and Victim
    graph.attackers->insert(attacker_asn);
    graph.victims->insert(victim_asn);
    
    // Set policies
    dynamic_cast<ROVppAS *>(graph.ases->find(77)->second)->add_policy(policy_type);
    dynamic_cast<ROVppAS *>(graph.ases->find(78)->second)->add_policy(policy_type);

    return graph;
}


// TODO: Function is incomplete, Do not use
/** Creates a ROVppASGraph object that is the same as figure 1 in the paper.
 * 
 * Distinct ASes: 32, 77, 11, 55, 54, 44, 33, 56, 99, 666
 * Victim: 99
 * Attacker: 666
 *
 * Expected outcomes:
 *  Hijacked:
 *  NotHijacked:
 *  Disconnected:
 *
 * @return ROVppASGraph object setup like figure 1
 */
ROVppASGraph figure_2_graph(uint32_t policy_type) {
    // TODO: function is incomplete needs
    ROVppASGraph graph = ROVppASGraph();
    uint32_t attacker_asn = 666;
    uint32_t victim_asn = 99;
    
    // Set Relationships
    // AS 44
    add_two_way_relationship(&graph, 44, 77, AS_REL_PROVIDER);
    add_two_way_relationship(&graph, 44, 54, AS_REL_PROVIDER);
    add_two_way_relationship(&graph, 44, 56, AS_REL_PROVIDER);
    add_two_way_relationship(&graph, 44, 666, AS_REL_PROVIDER);
    // AS 77
    add_two_way_relationship(&graph, 77, 11, AS_REL_PROVIDER);
    // AS 11
    add_two_way_relationship(&graph, 11, 32, AS_REL_PROVIDER);
    add_two_way_relationship(&graph, 11, 33, AS_REL_PROVIDER);
    // AS 54
    add_two_way_relationship(&graph, 54, 55, AS_REL_PROVIDER);
    // AS 55
    add_two_way_relationship(&graph, 55, 11, AS_REL_PROVIDER);
    // AS 56
    add_two_way_relationship(&graph, 56, 99, AS_REL_PROVIDER);
    
    // Set Attacker and Victim
    graph.attackers->insert(attacker_asn);
    graph.victims->insert(victim_asn);
    
    // Set policies
    dynamic_cast<ROVppAS *>(graph.ases->find(77)->second)->add_policy(policy_type);
    dynamic_cast<ROVppAS *>(graph.ases->find(33)->second)->add_policy(policy_type);
    dynamic_cast<ROVppAS *>(graph.ases->find(32)->second)->add_policy(ROVPPAS_TYPE_ROV);

    return graph;
}


/////////////////////////////////////////////////
// Tests
/////////////////////////////////////////////////

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

/** Test pass_rov which should fail if the origin of the announcement is an
 *  attacker. 
 *
 * @return True if successful, otherwise false
 */
bool test_rovpp_pass_rov() {
    ROVppASGraph graph = ROVppASGraph();
    graph.add_relationship(1, 2, AS_REL_PROVIDER);
    graph.add_relationship(2, 1, AS_REL_CUSTOMER);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    Announcement ann = Announcement(13796, p.addr, p.netmask, 22742);

    // no attackers should pass all announcements
    if (dynamic_cast<ROVppAS*>(graph.ases->find(1)->second)->pass_rov(ann) != true) {
        return false;
    }

    // not an attacker, should pass
    graph.attackers->insert(666);
    if (dynamic_cast<ROVppAS*>(graph.ases->find(1)->second)->pass_rov(ann) != true) {
        return false;
    }

    // 13796 is attacker, should fail
    graph.attackers->insert(13796);
    if (dynamic_cast<ROVppAS*>(graph.ases->find(1)->second)->pass_rov(ann) != false) {
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
    e.give_ann_to_as_path(as_path, p, 2, 0);

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
    e.give_ann_to_as_path(as_path_b, p, 1, 0);

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
    e.give_ann_to_as_path(as_path, p, 0, 0);
    delete as_path;

    // Check to providers
    e.send_all_announcements(2, true, false, false);
    if (!(e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
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
    if (!(e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(4)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(6)->second->loc_rib->size() == 0 &&
          e.graph->ases->find(7)->second->loc_rib->size() == 0)) {
        std::cerr << "Err sending to peers" << std::endl;
        return false;
    }

    // Check to customers
    e.send_all_announcements(2, false, false, true);
    if (!(e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
          e.graph->ases->find(4)->second->loc_rib->size() == 1 &&
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

/** Test adding a AS relationship to graph by building this test graph.
 *  AS2 is a provider to AS1, which is a peer to AS3.
 *
 *  AS2
 *   |
 *   V
 *  AS1---AS3
 * 
 * @return true if successful, otherwise false.
 */
bool test_rovpp_add_relationship(){
    ROVppASGraph graph = ROVppASGraph();
    graph.add_relationship(1, 2, AS_REL_PROVIDER);
    graph.add_relationship(2, 1, AS_REL_CUSTOMER);
    graph.add_relationship(1, 3, AS_REL_PEER);
    graph.add_relationship(3, 1, AS_REL_PEER);
    if (*graph.ases->find(1)->second->providers->find(2) != 2) {
        return false;
    }
    if (*graph.ases->find(1)->second->peers->find(3) != 3) {
        return false;
    }
    if (*graph.ases->find(2)->second->customers->find(1) != 1) {
        return false;
    }
    if (*graph.ases->find(3)->second->peers->find(1) != 1) {
        return false;
    }
    return true;
}

/** Test translating a ASN into it's supernode Identifier
 *
 * @return true if successful, otherwise false.
 */
bool test_rovpp_translate_asn(){
    ROVppASGraph graph = ROVppASGraph();
    // add_relation(Target, Neighber, Relation of neighbor to target)
    // Cycle 
    graph.add_relationship(2, 1, AS_REL_PROVIDER);
    graph.add_relationship(1, 2, AS_REL_CUSTOMER);
    graph.add_relationship(1, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 1, AS_REL_CUSTOMER);
    graph.add_relationship(3, 2, AS_REL_PROVIDER);
    graph.add_relationship(2, 3, AS_REL_CUSTOMER);
    // Customer
    graph.add_relationship(5, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 5, AS_REL_CUSTOMER);
    graph.add_relationship(6, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 6, AS_REL_CUSTOMER);
    // Peer
    graph.add_relationship(4, 3, AS_REL_PEER);
    graph.add_relationship(3, 4, AS_REL_PEER);
    graph.tarjan();
    graph.combine_components();

    if (graph.translate_asn(1) != 1 ||
        graph.translate_asn(2) != 1 ||
        graph.translate_asn(3) != 1)
        return false;
    if (graph.translate_asn(4) != 4 ||
        graph.translate_asn(5) != 5 ||
        graph.translate_asn(6) != 6)
        return false;
    return true;
}

/** Test assignment of ranks to each AS in the graph. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *   / \
 *  2   3--4
 *     / \
 *    5   6
 *
 * @return true if successful, otherwise false.
 */
bool test_rovpp_decide_ranks(){
    ROVppASGraph graph = ROVppASGraph();
    graph.add_relationship(2, 1, AS_REL_PROVIDER);
    graph.add_relationship(1, 2, AS_REL_CUSTOMER);
    graph.add_relationship(3, 1, AS_REL_PROVIDER);
    graph.add_relationship(1, 3, AS_REL_CUSTOMER);
    graph.add_relationship(5, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 5, AS_REL_CUSTOMER);
    graph.add_relationship(6, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 6, AS_REL_CUSTOMER);
    graph.add_relationship(4, 3, AS_REL_PEER);
    graph.add_relationship(3, 4, AS_REL_PEER);
    graph.decide_ranks();
    int num_systems = 6;
    int acc = 0;
    for (auto set : *graph.ases_by_rank) {
        acc += set->size();
    }
    if (acc != num_systems) {
        std::cerr << "Number of ASes in ases_by_rank != total number of ASes." << std::endl;
        return false;
    }
    if (graph.ases->find(1)->second->rank == 2 &&
        graph.ases->find(2)->second->rank == 0 &&
        graph.ases->find(3)->second->rank == 1 &&
        graph.ases->find(4)->second->rank == 0 &&
        graph.ases->find(5)->second->rank == 0 &&
        graph.ases->find(6)->second->rank == 0) {
        return true;
    }
    return false;
}

/** Test removing stub ASes from the graph. 
 * 
 *    1
 *   / \
 *  2   3--4
 *     / \
 *    5   6
 *
 * @return true if successful, otherwise false.
 */
bool test_rovpp_remove_stubs(){
    ROVppASGraph graph = ROVppASGraph();
    SQLQuerier *querier = new SQLQuerier("mrt_announcements", "test_results", "test_results", "test_results_d");
    graph.add_relationship(2, 1, AS_REL_PROVIDER);
    graph.add_relationship(1, 2, AS_REL_CUSTOMER);
    graph.add_relationship(3, 1, AS_REL_PROVIDER);
    graph.add_relationship(1, 3, AS_REL_CUSTOMER);
    graph.add_relationship(5, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 5, AS_REL_CUSTOMER);
    graph.add_relationship(6, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 6, AS_REL_CUSTOMER);
    graph.add_relationship(4, 3, AS_REL_PEER);
    graph.add_relationship(3, 4, AS_REL_PEER);
    graph.remove_stubs(querier);
    delete querier;
    // Stub removal
    if (graph.ases->find(2) != graph.ases->end() ||
        graph.ases->find(5) != graph.ases->end() ||
        graph.ases->find(6) != graph.ases->end()) {
        std::cerr << "Failed stubs removal check." << std::endl;
        return false;
    }
    // Stub translation
    if (graph.translate_asn(2) != 1 ||
        graph.translate_asn(5) != 3 ||
        graph.translate_asn(6) != 3) {
        std::cerr << "Failed stubs translation check." << std::endl;
        return false;
    }
    return true;
}



/** Test deterministic randomness within the AS scope.
 *
 * @return True if successful, otherwise false
 */
bool test_rovpp_get_random(){
    // Check randomness
    ROVppASGraph *as_graph = new ROVppASGraph();
    ROVppAS *as_a = new ROVppAS(832, as_graph->attackers);
    ROVppAS *as_b = new ROVppAS(832, as_graph->attackers);
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
bool test_rovpp_add_neighbor(){
    ROVppAS as = ROVppAS(1);
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
bool test_rovpp_remove_neighbor(){
    ROVppAS as = ROVppAS(1);
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
bool test_rovpp_receive_announcements(){
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
    ROVppAS as = ROVppAS(1);
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

/** Test receive_announcements if the AS is running ROV. 
 *
 * @return true if successful.
 */
bool test_rovpp_rov_receive_announcements(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    std::vector<Announcement> vect = std::vector<Announcement>();
    vect.push_back(ann);
    ann.prefix.addr = 0x321C9F00;
    ann.prefix.netmask = 0xFFFFFF00;
    ann.origin = 666;
    vect.push_back(ann);
    ROVppAS as = ROVppAS(1);
    // add the attacker and set the policy to ROV
    as.attackers = new std::set<uint32_t>();
    as.attackers->insert(666);
    as.add_policy(ROVPPAS_TYPE_ROV);
    as.receive_announcements(vect);
    delete as.attackers;
    if (as.ribs_in->size() != 2) { return false; }
    return true;
}


/** Test directly adding an announcement to the loc_rib map.
 *
 * @return true if successful.
 */
bool test_rovpp_process_announcement(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    // this function should make a copy of the announcement
    // if it does not, it is incorrect
    ROVppAS as = ROVppAS(1);
    as.process_announcement(ann);
    Prefix<> old_prefix = ann.prefix;
    ann.prefix.addr = 0x321C9F00;
    ann.prefix.netmask = 0xFFFFFF00;
    Prefix<> new_prefix = ann.prefix;
    as.process_announcement(ann);
    if (new_prefix != as.loc_rib->find(ann.prefix)->second.prefix ||
        old_prefix != as.loc_rib->find(old_prefix)->second.prefix) {
        return false;
    }

    // Check priority
    Prefix<> p = Prefix<>("1.1.1.0", "255.255.255.0");
    std::vector<uint32_t> x;
    Announcement a1 = Announcement(111, p.addr, p.netmask, 199, 222, 0, x);
    Announcement a2 = Announcement(111, p.addr, p.netmask, 298, 223, 0, x);
    as.process_announcement(a1);
    as.process_announcement(a2);
    if (as.loc_rib->find(p)->second.received_from_asn != 223 ||
        as.depref_anns->find(p)->second.received_from_asn != 222) {
        std::cerr << "Failed best path inference priority check." << std::endl;
        return false;
    }    

    // Check new best announcement
    Announcement a3 = Announcement(111, p.addr, p.netmask, 299, 224, 0, x);
    as.process_announcement(a3);
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
bool test_rovpp_process_announcements(){
    Announcement ann1 = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    Prefix<> ann1_prefix = ann1.prefix;
    Announcement ann2 = Announcement(13796, 0x321C9F00, 0xFFFFFF00, 22742);
    Prefix<> ann2_prefix = ann2.prefix;
    ROVppAS as = ROVppAS(1);
    // build a vector of announcements
    std::vector<Announcement> vect = std::vector<Announcement>();
    ann1.priority = 100;
    ann2.priority = 200;
    ann2.from_monitor = true;
    vect.push_back(ann1);
    vect.push_back(ann2);

    // does it work if loc_rib is empty?
    as.receive_announcements(vect);
    as.process_announcements();
    if (as.loc_rib->find(ann1_prefix)->second.priority != 100) {
        std::cerr << "Failed to add an announcement to an empty map" << std::endl;
        return false;
    }
    
    // higher priority should overwrite lower priority
    vect.clear();
    ann1.priority = 290;
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements();
    if (as.loc_rib->find(ann1_prefix)->second.priority != 290) {
        std::cerr << "Higher priority announcements should overwrite lower priority ones." << std::endl;
        return false;
    }
    
    // lower priority should not overwrite higher priority
    vect.clear();
    ann1.priority = 200;
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements();
    if (as.loc_rib->find(ann1_prefix)->second.priority != 290) {
        std::cerr << "Lower priority announcements should not overwrite higher priority ones." << std::endl;
        return false;
    }

    // one more test just to be sure
    vect.clear();
    ann1.priority = 299;
    vect.push_back(ann1);
    as.receive_announcements(vect);
    as.process_announcements();
    if (as.loc_rib->find(ann1_prefix)->second.priority != 299) {
        std::cerr << "How did you manage to fail here?" << std::endl;
        return false;
    }

    // make sure ann2 doesn't get overwritten, ever, even with higher priority
    vect.clear();
    ann2.priority = 300;
    vect.push_back(ann2);
    as.receive_announcements(vect);
    as.process_announcements();
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
bool test_rovpp_clear_announcements(){
    Announcement ann = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    // AS must hve a non-zero ASN in order to accept this announcement
    ROVppAS as = ROVppAS(1);
    as.process_announcement(ann);
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
bool test_rovpp_already_received(){
    Announcement ann1 = Announcement(13796, 0x89630000, 0xFFFF0000, 22742);
    Announcement ann2 = Announcement(13796, 0x321C9F00, 0xFFFFFF00, 22742);
    ROVppAS as = ROVppAS(1);
    // if receive_announcement is broken, this test will also be broken
    as.process_announcement(ann1);
    if (as.already_received(ann1) && !as.already_received(ann2)) {
        return true;
    }
    return false;
}

/** Test the constructor for the ROVppAnnouncement struct
 *
 * @ return True for success
 */
bool test_rovpp_announcement(){
    std::vector<uint32_t> x;
    ROVppAnnouncement ann = ROVppAnnouncement(111, 0x01010101, 0xffffff00, 0, 222, 100, 1, x);
    if (ann.origin != 111 
        || ann.prefix.addr != 0x01010101 
        || ann.prefix.netmask != 0xffffff00 
        || ann.received_from_asn != 222 
        || ann.priority != 0 
        || ann.from_monitor != false 
        || ann.tstamp != 100
        || ann.policy_index != 1) {
        return false;
    }
    
    ann = ROVppAnnouncement(111, 0x01010101, 0xffffff00, 262, 222, 100, 1, x, true);
    if (ann.origin != 111 
        || ann.prefix.addr != 0x01010101 
        || ann.prefix.netmask != 0xffffff00 
        || ann.received_from_asn != 222 
        || ann.priority != 262 
        || ann.from_monitor != true 
        || ann.tstamp != 100
        || ann.policy_index != 1) {
        return false;
    }

    return true;
}

/**
 * [test_best_alternative_route description]
 * The Store:
 * 
 * @return [description]
 */
bool test_best_alternative_route_chosen() {
    // Initialize AS
    ROVppAS as = ROVppAS(1);
    as.attackers = new std::set<uint32_t>();
    
    uint32_t attacker_asn = 666;
    uint32_t victim_asn = 99;
    
    // Initialize Attacker set
    as.attackers->insert(attacker_asn); 
    // Set the policy
    as.add_policy(ROVPPAS_TYPE_ROVPP);  // ROVpp0.1
    
    // Announcements from victim
    Prefix<> p1 = Prefix<>("1.2.0.0", "255.255.0.0");
    // The difference between the following three is the received_from_asn
    // Notice the priorities
    // The Announcements will be handled in this order
    std::vector<uint32_t> x;
    Announcement a1 = Announcement(victim_asn, p1.addr, p1.netmask, 222, 11, 0, x);  // If done incorrectly it will end up with this one
    a1.priority = 293;
    Announcement a2 = Announcement(victim_asn, p1.addr, p1.netmask, 222, 22, 0, x);  // or this one
    a2.priority = 292;
    Announcement a3 = Announcement(victim_asn, p1.addr, p1.netmask, 332, 33, 0, x);  // It should end up with this one if done correctly
    a3.priority = 291;
    
    // Subprefix Hijack
    Prefix<> p2 = Prefix<>("1.2.3.0", "255.255.0.0");
    // The difference between the following is the received_from_asn
    Announcement a4 = Announcement(attacker_asn, p2.addr, p2.netmask, 222, 11, 0, x);  // Should cause best_alternative_route to be called and end up with a2 in RIB
    a4.priority = 294;
    Announcement a5 = Announcement(attacker_asn, p2.addr, p2.netmask, 222, 22, 0, x);  // This cause it to go back a1 again, even though we just saw it conflicts with the previous annoucement (i.e. a4)
    a5.priority = 295;
    
    // Notice the victim's ann come first, then the attackers
    for (Announcement a : {a1, a2, a3, a4, a5}) {
        as.ribs_in->push_back(a);
    }
    
    as.process_announcements();
    
    // See if it ended up with the correct one
    Announcement selected_ann = as.loc_rib->find(p1)->second;
    // Note, this test is currently broken, correct behavior is choosing a1
    return selected_ann == a1;
}

/**
 * [test_best_alternative_route description]
 * The Store:
 * 
 * @return [description]
 */
bool test_best_alternative_route() {
    // Initialize AS
    ROVppAS as = ROVppAS(1);
    as.attackers = new std::set<uint32_t>();
    
    uint32_t attacker_asn = 666;
    uint32_t victim_asn = 99;
    
    // Initialize Attacker set
    as.attackers->insert(attacker_asn); 
    // Set the policy
    as.add_policy(ROVPPAS_TYPE_ROVPP);  // ROVpp0.1
    
    // Announcements from victim
    Prefix<> p1 = Prefix<>("1.2.0.0", "255.255.0.0");
    // The difference between the following three is the received_from_asn
    // Notice the priorities
    // The Announcements will be handled in this order
    std::vector<uint32_t> x;
    Announcement a1 = Announcement(victim_asn, p1.addr, p1.netmask, 222, 11, 0, x);  // If done incorrectly it will end up with this one
    a1.priority = 293;
    Announcement a2 = Announcement(victim_asn, p1.addr, p1.netmask, 222, 22, 0, x);  // or this one
    a2.priority = 292;
    Announcement a3 = Announcement(victim_asn, p1.addr, p1.netmask, 332, 33, 0, x);  // It should end up with this one if done correctly
    a3.priority = 291;
    
    // Subprefix Hijack
    Prefix<> p2 = Prefix<>("1.2.3.0", "255.255.0.0");
    // The difference between the following is the received_from_asn
    Announcement a4 = Announcement(attacker_asn, p2.addr, p2.netmask, 222, 11, 0, x);  // Should cause best_alternative_route to be called and end up with a2 in RIB
    a4.priority = 294;
    Announcement a5 = Announcement(attacker_asn, p2.addr, p2.netmask, 222, 22, 0, x);  // This cause it to go back a1 again, even though we just saw it conflicts with the previous annoucement (i.e. a4)
    a5.priority = 295;
    
    // Notice the victim's ann come first, then the attackers
    for (Announcement a : {a1, a2, a3, a4, a5}) {
        as.ribs_in->push_back(a);
    }
    
    if (!(as.best_alternative_route(a4) == a3) ||
        !(as.best_alternative_route(a5) == a3)) {
        return false;
    }
    return true; 
}

/** Test tiebreak override. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 *    
 *    2
 *   /|\
 *  1 4 3 
 *    |
 *    5
 *
 *  Announcements are seeded at 1 and 3. Pass/Fail is determined by what 5 receives.  
 */
bool test_rovpp_tiebreak_override() {
    ROVppExtrapolator e = ROVppExtrapolator();
    // disable random tiebreaks, instead use lowest ASN
    e.random = false;
    e.graph->add_relationship(1, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 1, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 4, AS_REL_PROVIDER);
    e.graph->add_relationship(4, 5, AS_REL_CUSTOMER);
    e.graph->decide_ranks();

    std::vector<uint32_t> *as_path1 = new std::vector<uint32_t>();
    as_path1->push_back(1);
    std::vector<uint32_t> *as_path2 = new std::vector<uint32_t>();
    as_path2->push_back(3);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    e.give_ann_to_as_path(as_path1, p, 2, 0);
    e.give_ann_to_as_path(as_path2, p, 2, 0);

    e.propagate_up();
    e.propagate_down();

    // confirm 5 has 1's announcement
    if(!(e.graph->ases->find(5)->second->loc_rib->find(p)->second.origin == 1)) {
        std::cerr << "Tiebreak override failed step 1\n";
        return false;
    }

    // set override. normally this AS would lose the tiebreak
    e.graph->ases->find(2)->second->loc_rib->find(p)->second.origin = 3;
    e.graph->ases->find(2)->second->loc_rib->find(p)->second.received_from_asn = 3;
    e.graph->ases->find(2)->second->loc_rib->find(p)->second.tiebreak_override = 3;
    e.propagate_down();

    // confirm 5 has 3's announcement
    if(!(e.graph->ases->find(5)->second->loc_rib->find(p)->second.origin != 3)) {
        std::cerr << "Tiebreak override failed step 2\n";
        return false;
    }
    return true;
}

/** Test withdrawals.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 *
 */
bool test_withdrawal() {
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
    as_path->push_back(5);
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
   // e.graph->ases->find(5)->second->process_announcement(ann);
    e.give_ann_to_as_path(as_path, p, 2, 0);
    e.propagate_up();
    e.propagate_down();

    // Check all announcements are propagted
    if (!(e.graph->ases->find(1)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(2)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(3)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(4)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(5)->second->loc_rib->size() == 1 &&
        e.graph->ases->find(6)->second->loc_rib->size() == 1)) {
        std::cerr << "Announcements did not propagate as expected\n";
        return false;
    }
    
    // Make withdrawal
    Announcement copy = e.graph->ases->find(5)->second->loc_rib->find(p)->second;
    e.graph->ases->find(5)->second->loc_rib->erase(e.graph->ases->find(5)->second->loc_rib->find(p));
    copy.withdraw = true;
    e.graph->ases->find(5)->second->withdrawals->push_back(copy);
    //e.graph->ases->find(5)->second->process_announcement(ann);
    e.propagate_up();
    e.propagate_down();
    e.propagate_up();
    e.propagate_down();

    std::cerr << e.graph->ases->find(1)->second->loc_rib->size() << std::endl; 
    std::cerr << e.graph->ases->find(2)->second->loc_rib->size() << std::endl;
    std::cerr << e.graph->ases->find(3)->second->loc_rib->size() << std::endl;
    std::cerr << e.graph->ases->find(4)->second->loc_rib->size() << std::endl;
    std::cerr << e.graph->ases->find(5)->second->loc_rib->size() << std::endl;
    std::cerr << e.graph->ases->find(6)->second->loc_rib->size() << std::endl;
        
    if (!(e.graph->ases->find(1)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(2)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(3)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(4)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(5)->second->loc_rib->size() == 0 &&
        e.graph->ases->find(6)->second->loc_rib->size() == 0)) {
        std::cerr << "Announcements did not withdraw as expected\n";
        return false;
    }

    delete as_path;
    return true;
}



/** Testing Blackholing (i.e. when only a blackhole is produced)
 *
 * Blackholing produces a blackhole if it has no other safe
 * alternative route.
 *
 * The topology of this graph is in the paper (figure 1).
 * 
 *
 * @return true if successful, otherwise false.
 */
// bool test_rovpp_blackholing() {
//     // Constants
//     uint32_t attacker_asn = 666;
//     uint32_t victim_asn = 99;
//     Prefix<> attacker_prefix = Prefix<>("1.2.3.0", "255.255.255.0");
//     Prefix<> victim_prefix = Prefix<>("1.2.0.0", "255.255.0.0");
// 
//     // Create Graph
//     Extrapolator e = Extrapolator();
//     // AS44
//     add_two_way_relationship(e.graph, 44, 77, AS_REL_PROVIDER, AS_TYPE_BGP, AS_TYPE_ROVPP);
//     add_two_way_relationship(e.graph, 44, 78, AS_REL_PROVIDER, AS_TYPE_BGP, AS_TYPE_BGP);
//     add_two_way_relationship(e.graph, 44, 99, AS_REL_PROVIDER, AS_TYPE_BGP, AS_TYPE_BGP);
//     add_two_way_relationship(e.graph, 44, 666, AS_REL_PROVIDER, AS_TYPE_BGP, AS_TYPE_BGP);
//     // AS77
//     add_two_way_relationship(e.graph, 77, 11, AS_REL_PROVIDER, AS_TYPE_ROVPP, AS_TYPE_BGP);
//     // AS78
//     add_two_way_relationship(e.graph, 78, 12, AS_REL_PROVIDER, AS_TYPE_ROVPP, AS_TYPE_BGP);
//     // AS88
//     add_two_way_relationship(e.graph, 88, 78, AS_REL_PROVIDER, AS_TYPE_BGP, AS_TYPE_ROVPP);
//     add_two_way_relationship(e.graph, 88, 86, AS_REL_PROVIDER, AS_TYPE_BGP, AS_TYPE_BGP);
//     // AS86
//     add_two_way_relationship(e.graph, 86, 99, AS_REL_PROVIDER, AS_TYPE_BGP, AS_TYPE_BGP);
//     // Prepare Graph
//     e.graph->decide_ranks();
// 
//     // Create and Propogate Announcements
//     // Attacker Announcement
//     Announcement attacker_ann = Announcement(attacker_asn, attacker_prefix.addr, attacker_prefix.netmask, 0);
//     attacker_ann.from_monitor = true;
//     attacker_ann.priority = 299;
//     // Propogate Attacker Announcement
//     e.graph->ases->find(attacker_asn)->second->receive_announcement(attacker_ann);
//     e.propagate_up();
//     e.propagate_down();
// 
//     // Victim Announcement
//     Announcement victim_ann = Announcement(victim_asn, victim_prefix.addr, victim_prefix.netmask, 0);
//     victim_ann.from_monitor = true;
//     victim_ann.priority = 299;
//     // Propogate Victim Announcement
//     e.graph->ases->find(victim_asn)->second->receive_announcement(victim_ann);
//     e.propagate_up();
//     e.propagate_down();
// 
//     // Check the results
//     // Check if attacker succeeded where it shouldn't have
//     std::vector<uint32_t> saved_ones = {77, 11, 78, 12};
//     return !(check_infected_vector(e.graph, saved_ones, attacker_prefix));
// }

/** Test full path propagation.
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2---3
 *   /|    \
 *  4 5--6  7
 */
bool test_rovpp_full_path() {
    // TODO finish this 
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
    std::vector<uint32_t> *as_path = new std::vector<uint32_t>();
    as_path->push_back(5);
    
    e.give_ann_to_as_path(as_path, p, 1, 0);
    e.propagate_up();
    e.propagate_down();

    // Check if seeded path is present
    if (e.graph->ases->find(5)->second->loc_rib->find(p)->second.as_path.size() != 1) {
        std::cerr << "Failed seeding full AS path." << '\n';
        for (auto const& i: e.graph->ases->find(5)->second->loc_rib->find(p)->second.as_path) {
            std::cerr << i;
        }
        std::cerr << '\n';
        return false;
    }
    
    // Check if propagated paths are correct]
    // Paths are stored in reverse order
    std::vector<uint32_t> path_1{ 5, 2 };
    std::vector<uint32_t> path_2{ 5 };
    std::vector<uint32_t> path_3{ 5, 2 };
    std::vector<uint32_t> path_4{ 5, 2 };
    std::vector<uint32_t> path_6{ 5 };
    std::vector<uint32_t> path_7{ 5, 2, 3 };

    if (e.graph->ases->find(1)->second->loc_rib->find(p)->second.as_path.size() != 2 || 
        e.graph->ases->find(2)->second->loc_rib->find(p)->second.as_path.size() != 1 ||
        e.graph->ases->find(3)->second->loc_rib->find(p)->second.as_path.size() != 2 ||
        e.graph->ases->find(4)->second->loc_rib->find(p)->second.as_path.size() != 2 ||
        e.graph->ases->find(6)->second->loc_rib->find(p)->second.as_path.size() != 1 ||
        e.graph->ases->find(7)->second->loc_rib->find(p)->second.as_path.size() != 3) {
        std::cerr << "Failed propagating full AS path." << '\n';
        return false;
    }
    
    if (e.graph->ases->find(1)->second->loc_rib->find(p)->second.as_path != path_1) { 
        std::cerr << "Failed propagating full AS path." << '\n';
        for (auto const& i: e.graph->ases->find(1)->second->loc_rib->find(p)->second.as_path) {
            std::cerr << i;
        }
        std::cerr << '\n';
        for (auto const& i: path_1) {
            std::cerr << i;
        }
        std::cerr << '\n';
        return false;
    }
    if (e.graph->ases->find(2)->second->loc_rib->find(p)->second.as_path != path_2) { 
        std::cerr << "Failed propagating full AS path." << '\n';
        for (auto const& i: e.graph->ases->find(2)->second->loc_rib->find(p)->second.as_path) {
            std::cerr << i;
        }
        std::cerr << '\n';
        for (auto const& i: path_2) {
            std::cerr << i;
        }
        std::cerr << '\n';
        return false;
    }
    if (e.graph->ases->find(3)->second->loc_rib->find(p)->second.as_path != path_3) { 
        std::cerr << "Failed propagating full AS path." << '\n';
        for (auto const& i: e.graph->ases->find(3)->second->loc_rib->find(p)->second.as_path) {
            std::cerr << i;
        }
        std::cerr << '\n';
        for (auto const& i: path_3) {
            std::cerr << i;
        }
        std::cerr << '\n';
        return false;
    }
    if (e.graph->ases->find(4)->second->loc_rib->find(p)->second.as_path != path_4) { 
        std::cerr << "Failed propagating full AS path." << '\n';
        for (auto const& i: e.graph->ases->find(4)->second->loc_rib->find(p)->second.as_path) {
            std::cerr << i;
        }
        std::cerr << '\n';
        for (auto const& i: path_4) {
            std::cerr << i;
        }
        std::cerr << '\n';
        return false;
    }
    if (e.graph->ases->find(6)->second->loc_rib->find(p)->second.as_path != path_6) { 
        std::cerr << "Failed propagating full AS path." << '\n';
        for (auto const& i: e.graph->ases->find(6)->second->loc_rib->find(p)->second.as_path) {
            std::cerr << i;
        }
        std::cerr << '\n';
        for (auto const& i: path_6) {
            std::cerr << i;
        }
        std::cerr << '\n';
        return false;
    }
    if (e.graph->ases->find(7)->second->loc_rib->find(p)->second.as_path != path_7) { 
        std::cerr << "Failed propagating full AS path." << '\n';
        for (auto const& i: e.graph->ases->find(7)->second->loc_rib->find(p)->second.as_path) {
            std::cerr << i;
        }
        std::cerr << '\n';
        for (auto const& i: path_7) {
            std::cerr << i;
        }
        std::cerr << '\n';
        return false;
    }

    return true;
}


bool test_tiny_hash() {
    std::cout << (int) ROVppAS(1).tiny_hash(1) << std::endl;
    std::cout << (int) ROVppAS(2).tiny_hash(2) << std::endl;
    std::cout << (int) ROVppAS(3).tiny_hash(3) << std::endl;
    std::cout << (int) ROVppAS(4).tiny_hash(4) << std::endl;
    std::cout << (int) ROVppAS(5).tiny_hash(5) << std::endl;
    return true;
}
