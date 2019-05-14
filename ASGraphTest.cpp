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
 * @return true of successful, otherwise false.
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


/**
 *
 * @return true of successful, otherwise false.
 */
bool test_translate_asn(){
    return true;
}


/** Test generation of ASGraph from database tables.
 *
 * @return true of successful, otherwise false.
 */
bool test_create_graph_from_db(){
    return true;
}


/** Test assignment of ranks to each AS in the graph.
 *
 * @return true of successful, otherwise false.
 */
bool test_decide_ranks(){
    return true;
}


/** Test removing stub ASes from the graph. 
 *
 * @return true of successful, otherwise false.
 */
bool test_remove_stubs(){
    return true;
}


/**
 *
 * @return true of successful, otherwise false.
 */
bool test_tarjan(){ // includes tarjan_helper()
    return true;
}


/**
 *
 * @return true of successful, otherwise false.
 */
bool test_combine_components(){
    return true;
}


/** 
 *
 * @return true of successful, otherwise false.
 */
bool test_save_stubs_to_db(){
    return true;
}


/** 
 *
 * @return true of successful, otherwise false.
 */
bool test_save_non_stubs_to_db(){
    return true;
}


/**
 *
 * @return true of successful, otherwise false.
 */
bool test_save_supernodes_to_db(){
    return true;
}
