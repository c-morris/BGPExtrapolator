#include <iostream>
#include "ASGraph.h"
#include "AS.h"

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


/**
 *
 * @return true if successful, otherwise false.
 */
bool test_tarjan(){ // includes tarjan_helper()
    return true;
}


/**
 *
 * @return true if successful, otherwise false.
 */
bool test_combine_components(){
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
