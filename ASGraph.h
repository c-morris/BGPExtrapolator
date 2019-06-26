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
#include "Prefix.h"
#include "Announcement.h"


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
    // ROV and ROVpp related properties
    std::set<uint32_t> *rov_asn_set;  // Set of ROV ASNs
    std::set<uint32_t> *rovpp_asn_set;  // Set of ROVpp ASNs
    // Set of nodes that implement a particular version of ROVpp
    std::set<uint32_t> *rovpp_asn_set_using_b;  // Only blackholes enabled
    std::set<uint32_t> *rovpp_asn_set_using_bf;  // Blackholes and Friends enabled
    std::set<uint32_t> *rovpp_asn_set_using_bfp;  // Blackholes, Friends, and Preferences enabled
    // Simulation variables
    std::uint32_t attacker_asn;
    std::uint32_t victim_asn;
    std::string victim_prefix;
    bool negative_anns_enabled;
    bool friends_enabled;
    bool preferences_enabled;
    // Friends Repository (will fill with friendly warnings)
    std::map<Prefix<>, std::set<Announcement>> hazard_bulletin;
    std::vector<uint32_t> *hazard_subscribers;


    ASGraph();
    ASGraph(std::uint32_t attacker_asn, std::uint32_t victim_asn, std::string victim_prefix,
    bool enable_negative_anns, bool enable_friends, bool enable_preferences);
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
    void load_rov_ases(SQLQuerier *querier);
    void load_rovpp_ases(SQLQuerier *querier);
    AS * get_as_with_asn(uint32_t asn);
    bool implements_rovpp(uint32_t asn);

    uint32_t translate_asn(uint32_t asn);
    void clear_announcements();
    // ROVpp Related methods
    void subscribe_to_hazards(uint32_t asn);  // TODO: Impelement
    void publish_harzard(Announcement hazard_ann, uint32_t publishing_asn);  // TODO: Impelement

    friend std::ostream& operator<<(std::ostream &os, const ASGraph& asg);
};

#endif
