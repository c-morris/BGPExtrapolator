#include <iostream>
#include "ASGraph.h"
#include "AS.h"
#include "RanGraph.h"

/** Unit tests for ASGraph.h and ASGraph.cpp
 */

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
bool test_add_relationship(){
    ASGraph graph = ASGraph();
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


/** TODO
 *
 * @return true if successful, otherwise false.
 */
bool test_translate_asn(){
    return true;
}


/** Test generation of ASGraph from database tables.
 *
 * @return true if successful, otherwise false.
 */
bool test_create_graph_from_db(){
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
bool test_decide_ranks(){
    ASGraph graph = ASGraph();
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
        graph.ases->find(4)->second->rank == 1 &&
        graph.ases->find(5)->second->rank == 0 &&
        graph.ases->find(6)->second->rank == 0) {
        return true;
    }
    return false;
}


/** Test removing stub ASes from the graph. 
 *
 * @return true if successful, otherwise false.
 */
bool test_remove_stubs(){
    return true;
}


/** Test the Tarjan algorithm for detecting strongly connected components.
 * Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *   / \
 *  2<- 3--4
 *     / \
 *    5   6
 *
 * @return true if successful, otherwise false.
 */
bool test_tarjan(){ // includes tarjan_helper()
    ASGraph graph = ASGraph();
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
    if (graph.components->size() != 4) {
        return false;
    }
    // Verify the cycle was detected
    for (auto const& c : *graph.components) {
        if (c->size() > 1 && c->size() != 3)
            return false;
    }   
     
    // Multi supernode test
    ASGraph graph2 = ASGraph();
    // Cycle 10->11->12->10
    graph2.add_relationship(11, 10, AS_REL_PROVIDER);
    graph2.add_relationship(10, 11, AS_REL_CUSTOMER);
    graph2.add_relationship(12, 11, AS_REL_PROVIDER);
    graph2.add_relationship(11, 12, AS_REL_CUSTOMER);
    graph2.add_relationship(10, 12, AS_REL_PROVIDER);
    graph2.add_relationship(12, 10, AS_REL_CUSTOMER);
    // Cycle 13->14->15->13
    graph2.add_relationship(14, 13, AS_REL_PROVIDER);
    graph2.add_relationship(13, 14, AS_REL_CUSTOMER);
    graph2.add_relationship(15, 14, AS_REL_PROVIDER);
    graph2.add_relationship(14, 15, AS_REL_CUSTOMER);
    graph2.add_relationship(13, 15, AS_REL_PROVIDER);
    graph2.add_relationship(15, 13, AS_REL_CUSTOMER);
    // Peer
    graph2.add_relationship(11, 14, AS_REL_PEER);
    graph2.add_relationship(14, 11, AS_REL_PEER);
    graph2.tarjan();
    
    if (graph2.components->size() != 2) {
        return false;
    }
    // Verify the cycle was detected
    for (auto const& c : *graph2.components) {
        if (c->size() > 1 && c->size() != 3)
            return false;
    }
    
    ASGraph *graph3;
    graph3 = ran_graph(5, 10);
    std::cout << *graph3 << std::endl;
    graph3->tarjan();
    return true;
}


/** Test the combining of strongly connected components into supernodes.
 * 
 *        7
 *        |
 *        1
 *     /  |  \
 * 8--2 ->3-- 4
 *       / \
 *      5   6
 *
 *
 * @return true if successful, otherwise false.
 */
bool test_combine_components(){
     ASGraph graph = ASGraph();
    // Cycle 1->2->3->1
    graph.add_relationship(2, 1, AS_REL_PROVIDER);
    graph.add_relationship(1, 2, AS_REL_CUSTOMER);
    graph.add_relationship(3, 2, AS_REL_PROVIDER);
    graph.add_relationship(2, 3, AS_REL_CUSTOMER);
    graph.add_relationship(1, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 1, AS_REL_CUSTOMER);
    // Provider
    graph.add_relationship(1, 7, AS_REL_PROVIDER);
    graph.add_relationship(7, 1, AS_REL_CUSTOMER);
    graph.add_relationship(4, 1, AS_REL_PROVIDER);
    graph.add_relationship(1, 4, AS_REL_CUSTOMER);
    // Customer
    graph.add_relationship(5, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 5, AS_REL_CUSTOMER);
    graph.add_relationship(6, 3, AS_REL_PROVIDER);
    graph.add_relationship(3, 6, AS_REL_CUSTOMER);
    // Peer
    graph.add_relationship(4, 3, AS_REL_PEER);
    graph.add_relationship(3, 4, AS_REL_PEER);
    graph.add_relationship(8, 2, AS_REL_PEER);
    graph.add_relationship(2, 8, AS_REL_PEER);

    graph.tarjan();
    graph.combine_components();
    // Check for the supernode
    if (graph.component_translation->size() != 3) {
        std::cerr << "Incorrect translation size." << std::endl;
        return false;
    }
    // Verify lowest ASN is identifier
    for (auto c : *graph.component_translation) {
        if (c.second != 1) {
            std::cerr << "Incorrect translation ID for supernode." << std::endl;
            return false;
        }
    }

    // Find supernode AS
    auto SCC = graph.ases->find(1);
    AS *supernode = SCC->second;
    // Check supernode providers
    if (supernode->providers->find(7) == supernode->providers->end()) {
        std::cerr << "Incorrect supernode provider set." << std::endl;
        return false;
    }
    // Check supernode customers
    if (supernode->customers->find(4) == supernode->customers->end() ||
        supernode->customers->find(5) == supernode->customers->end() ||
        supernode->customers->find(6) == supernode->customers->end()) {
        std::cerr << "Incorrect supernode customer set." << std::endl;
        return false;
    }
    // Check supernode peers
    if (supernode->peers->find(8) == supernode->peers->end()) {
        std::cerr << "Incorrect supernode peer set." << std::endl;
        return false;
    }
    // Check supernode peers
    if (supernode->peers->find(4) != supernode->peers->end()) {
        std::cerr << "Incorrect supernode peer-customer override." << std::endl;
        return false;
    }


    // Multi supernode test
    ASGraph graph2 = ASGraph();
    // Cycle 10->11->12->10
    graph2.add_relationship(11, 10, AS_REL_PROVIDER);
    graph2.add_relationship(10, 11, AS_REL_CUSTOMER);
    graph2.add_relationship(12, 11, AS_REL_PROVIDER);
    graph2.add_relationship(11, 12, AS_REL_CUSTOMER);
    graph2.add_relationship(10, 12, AS_REL_PROVIDER);
    graph2.add_relationship(12, 10, AS_REL_CUSTOMER);
    // Cycle 13->14->15->13
    graph2.add_relationship(14, 13, AS_REL_PROVIDER);
    graph2.add_relationship(13, 14, AS_REL_CUSTOMER);
    graph2.add_relationship(15, 14, AS_REL_PROVIDER);
    graph2.add_relationship(14, 15, AS_REL_CUSTOMER);
    graph2.add_relationship(13, 15, AS_REL_PROVIDER);
    graph2.add_relationship(15, 13, AS_REL_CUSTOMER);
    // Peer
    graph2.add_relationship(11, 14, AS_REL_PEER);
    graph2.add_relationship(14, 11, AS_REL_PEER);
    
    graph2.add_relationship(12, 14, AS_REL_PEER);
    graph2.add_relationship(14, 12, AS_REL_PEER);
    
    graph2.add_relationship(12, 15, AS_REL_PEER);
    graph2.add_relationship(15, 12, AS_REL_PEER);

    // Provider
    //graph2.add_relationship(11, 15, AS_REL_PROVIDER);
    //graph2.add_relationship(15, 11, AS_REL_CUSTOMER);

    graph2.tarjan();
    graph2.combine_components();

    if (graph2.component_translation->size() != 6) {
        std::cerr << "Incorrect translation size: " << std::endl;
        return false;
    }

    // Find supernode AS
    std::cout << graph2 << std::endl;
    auto SCC1 = graph2.ases->find(10);
    auto SCC2 = graph2.ases->find(13);
    AS *supernode1 = SCC1->second;
    AS *supernode2 = SCC2->second;
    // Check supernode providers
    if (supernode1->providers->find(13) != supernode1->providers->end()) {
        std::cerr << "Incorrect supernode provider set." << std::endl;
        return false;
    }
    // Check supernode peers
    if (supernode1->peers->find(13) == supernode1->peers->end() ||
        supernode2->peers->find(10) == supernode2->peers->end()) {
        std::cerr << "Incorrect supernode peer set." << std::endl;
        return false;
    }
    for (auto node : *graph2.component_translation) {
        std::cout << node.first << " " << node.second << std::endl;
    }
    

    return true;
}


/** 
 *
 * @return true if successful, otherwise false.
 */
bool test_save_stubs_to_db(){
    return true;
}


/** 
 *
 * @return true if successful, otherwise false.
 */
bool test_save_non_stubs_to_db(){
    return true;
}


/**
 *
 * @return true if successful, otherwise false.
 */
bool test_save_supernodes_to_db(){
    return true;
}
