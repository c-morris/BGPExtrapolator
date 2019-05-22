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
    std::map<uint32_t, AS*> *ases; // map of ASN to AS object 
    std::vector<uint32_t> *ases_with_anns;
    std::vector<std::set<uint32_t>*> *ases_by_rank;
    std::vector<std::vector<uint32_t>*> *components;
    std::map<uint32_t, uint32_t> *stubs_to_parents;
    std::vector<uint32_t> *non_stubs;
    std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results; 
    //component_translation keeps key-value pairs for each ASN and the ASN
    //it became a part of due to strongly connected component combination.
    //This is used in Extrapolator.give_ann_to_as_path() where ASNs on an 
    //announcements AS_PATH need to be located.
    std::map<uint32_t, uint32_t> *component_translation;

    ASGraph();
    ~ASGraph();
    void add_relationship(uint32_t asn, uint32_t neighbor_asn, int relation);
    void decide_ranks();
    void process();
    void process(SQLQuerier *querier);
    void remove_stubs(SQLQuerier *querier);
    void tarjan();
    void tarjan_helper(AS *as, int &index, std::stack<AS*> &s);
    void printDebug();
    void combine_components();
    void create_graph_from_files();
    void create_graph_from_db(SQLQuerier *querier);
    void save_stubs_to_db(SQLQuerier *querier);
    void save_supernodes_to_db(SQLQuerier *querier);
    void save_non_stubs_to_db(SQLQuerier *querier);
    void to_graphviz(std::ostream &os);

    uint32_t translate_asn(uint32_t asn);
    void clear_announcements();

    friend std::ostream& operator<<(std::ostream &os, const ASGraph& asg);
};

#endif

