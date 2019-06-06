#ifndef ASGRAPH_H
#define ASGRAPH_H

#define AS_REL_PROVIDER 0
#define AS_REL_PEER 1
#define AS_REL_CUSTOMER 2

struct SQLQuerier;

#include <map>
#include <vector>
#include <stack>
#include <sys/stat.h>
#include <dirent.h>
#include <pqxx/pqxx>
#include "AS.h"
#include "SQLQuerier.h"
#include "TableNames.h"

struct ASGraph {
    std::map<uint32_t, AS*> *ases;                      // Map of ASN to AS object 
    std::vector<std::set<uint32_t>*> *ases_by_rank;     // Vector of ranks
    std::vector<std::vector<uint32_t>*> *components;    // Strongly connected components
    std::map<uint32_t, uint32_t> *component_translation;// Translate AS to supernode AS
    std::map<uint32_t, uint32_t> *stubs_to_parents;
    std::vector<uint32_t> *non_stubs;
    std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results; 

    ASGraph();
    ~ASGraph();
    // Propagation interaction 
    void clear_announcements();
    uint32_t translate_asn(uint32_t asn);
    // Graph setup
    void add_relationship(uint32_t asn, uint32_t neighbor_asn, int relation);
    void process(); // Remove?
    void process(SQLQuerier *querier); 
    void create_graph_from_db(SQLQuerier *querier);
    void remove_stubs(SQLQuerier *querier);
    void save_stubs_to_db(SQLQuerier *querier);
    void save_non_stubs_to_db(SQLQuerier *querier);
    void save_supernodes_to_db(SQLQuerier *querier);
    void decide_ranks();
    // Supernode generation
    void tarjan();
    void tarjan_helper(AS *as, int &index, std::stack<AS*> &s);
    void combine_components();
    // Misc
    void printDebug();
    void to_graphviz(std::ostream &os);
    friend std::ostream& operator<<(std::ostream &os, const ASGraph& asg);
};

#endif

