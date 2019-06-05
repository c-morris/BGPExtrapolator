#include <cstdint>
#include <iostream>
#include <set>
#include <vector>
#include <stack>
#include <algorithm>
#include <limits.h>
#include "ASGraph.h"
#include "AS.h"
#include "ROVAS.h"
#include "NegativeAnnouncement.h"

ASGraph::ASGraph() {
    ases = new std::map<uint32_t, AS*>;
    ases_by_rank = new std::vector<std::set<uint32_t>*>;
    rov_asn_set = new std::set<uint32_t>;
    components = new std::vector<std::vector<uint32_t>*>;
    component_translation = new std::map<uint32_t, uint32_t>;
    stubs_to_parents = new std::map<uint32_t, uint32_t>;
    non_stubs = new std::vector<uint32_t>;
    inverse_results = new std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*>;
}

ASGraph::ASGraph(std::uint32_t attacker_asn, std::uint32_t victim_asn, std::string victim_prefix) {
  this->attacker_asn = attacker_asn;
  this->victim_asn = victim_asn;
  this->victim_prefix = victim_prefix;

  ases = new std::map<uint32_t, AS*>;
  ases_by_rank = new std::vector<std::set<uint32_t>*>;
  rov_asn_set = new std::set<uint32_t>;
  components = new std::vector<std::vector<uint32_t>*>;
  component_translation = new std::map<uint32_t, uint32_t>;
  stubs_to_parents = new std::map<uint32_t, uint32_t>;
  non_stubs = new std::vector<uint32_t>;
  inverse_results = new std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*>;
}

ASGraph::~ASGraph() {
    for (auto const& as : *ases) {
        delete as.second;
    }
    delete ases;

    for (auto const& as : *ases_by_rank) {
        delete as;
    }
    delete ases_by_rank;

    for (auto const& c : *components) {
        delete c;
    }
    delete components;

    for (auto const& i : *inverse_results) {
        delete i.second;
    }
    delete inverse_results;

    delete component_translation;
    delete stubs_to_parents;
    delete non_stubs;
}


/** Adds an AS relationship to the graph.
 *
 * If the AS does not exist in the graph, it will be created.
 *
 * @param asn ASN of AS to add the relationship to
 * @param neighbor_asn ASN of neighbor.
 * @param relation AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
 */
//TODO probably make this do bi-directional assignment instead of calling this
//twice for each pair
void ASGraph::add_relationship(uint32_t asn, uint32_t neighbor_asn,
    int relation) {
    auto search = ases->find(asn);
    if (search == ases->end()) {
        // if AS not yet in graph, create it
        // What kind of AS is it (Regular BGP or ROV)?
        auto rov_search = rov_asn_set->find(asn);
        if (rov_search == rov_asn_set->end()) {  // Check if it's not in the ROV set
          // Create a BGP AS
          ases->insert(std::pair<uint32_t, AS*>(asn, new AS(asn, inverse_results)));
        } else {
          // Create a ROV AS
          ases->insert(std::pair<uint32_t, AS*>(asn, new ROVAS(asn, attacker_asn, victim_asn, victim_prefix, inverse_results, this)));
        }
        search = ases->find(asn);
    }
    search->second->add_neighbor(neighbor_asn, relation);
}


AS * ASGraph::get_as_with_asn(uint32_t asn) {
  auto search = ases->find(asn);
  if(search == ases->end()){
     return NULL;
  }
  return search->second;
}

/** Translates asn to asn of component it belongs to in graph.
 *
 * @return 0 if asn isn't found
 */
uint32_t ASGraph::translate_asn(uint32_t asn){
    auto search = component_translation->find(asn);
    if(search == component_translation->end()){
       return asn;
    }
    return search->second;
}


/** Process without removing stubs (doesn't need querier).
 */
void ASGraph::process(){
    tarjan();
    combine_components();
    decide_ranks();
    return;
}


/** Process with removing stubs (needs querier to save them).
 */
void ASGraph::process(SQLQuerier *querier){
    // remove_stubs(querier);  //  For Out simulations we don't need this
    tarjan();
    combine_components();

    // debug
    //std::ofstream outfile;
    //std::string fname = "graph";
    //fname += std::to_string(0);
    //fname += ".py";
    //outfile.open(fname, std::ios::out | std::ios::trunc);
    //outfile << "import graphviz\ndot = graphviz.Digraph()\n";
    //this->to_graphviz(outfile);
    //outfile << "dot.render('test-output/extrapolator.gv', view=True) \n";
    //outfile.close();

    save_supernodes_to_db(querier);
    decide_ranks();
    return;
}

/** Initializes the set of ASNs (rov_asn_set) variable, with entries from database.
*
*/
void ASGraph::load_rov_ases(SQLQuerier *querier) {
  pqxx::result rov_ases_result = querier->select_rov_ases(ASES_TABLE);
  for (pqxx::result::const_iterator c = rov_ases_result.begin(); c!=rov_ases_result.end(); ++c) {
    if (c["as_type"].as<std::string>() == "rov") {
      std::cout << "ROV AS " << c["asn"].as<uint32_t>() << std::endl;
      rov_asn_set->insert(c["asn"].as<uint32_t>());
    }
  }
}


/** Generates an ASGraph from relationship data in an SQL database based upon:
 *      1) A populated peers table
 *      2) A populated customer_providers table
 *
 * @param querier
 */
void ASGraph::create_graph_from_db(SQLQuerier *querier){
    //TODO add table names to config
    load_rov_ases(querier); // initialize map of ASes
    pqxx::result R = querier->select_from_table(PEERS_TABLE);
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        add_relationship(c["peer_as_1"].as<uint32_t>(),
                        c["peer_as_2"].as<uint32_t>(),AS_REL_PEER);
        add_relationship(c["peer_as_2"].as<uint32_t>(),
                        c["peer_as_1"].as<uint32_t>(),AS_REL_PEER);
    }
    R = querier->select_from_table(CUSTOMER_PROVIDER_TABLE);
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        add_relationship(c["customer_as"].as<uint32_t>(),c["provider_as"].as<uint32_t>(),AS_REL_PROVIDER);
        add_relationship(c["provider_as"].as<uint32_t>(),c["customer_as"].as<uint32_t>(),AS_REL_CUSTOMER);
    }

    process(querier);
    return;
}


/** Remove the stub ASes from the graph.
 *
 * @param querier
 */
void ASGraph::remove_stubs(SQLQuerier *querier){
    std::vector<AS*> to_remove;
    for (auto &as : *ases){
        if(as.second->peers->size() == 0 &&
           as.second->providers->size() == 1 &&
           (as.second->customers->size() == 0)) {// || as.second->customers->size() == 1)) {
            to_remove.push_back(as.second);
        } else {
            non_stubs->push_back(as.first);
        }
    }
    for (auto *as : to_remove) {
        // remove any edges to this stub from graph
        for(uint32_t provider_asn : *as->providers){
            auto iter = ases->find(provider_asn);
            if (iter != ases->end()) {
                AS* provider = iter->second;
                provider->customers->erase(as->asn);
            }
            stubs_to_parents->insert(std::pair<uint32_t,uint32_t>(
                        as->asn,provider_asn));
        }
        for(uint32_t peer_asn : *as->peers){
            auto iter = ases->find(peer_asn);
            if (iter != ases->end()) {
                AS * peer = iter->second;
                peer->peers->erase(as->asn);
            }
        }
        for(uint32_t customer_asn : *as->customers){
            auto iter = ases->find(customer_asn);
            if (iter != ases->end()) {
                AS * customer = iter->second;
                customer->providers->erase(as->asn);
            }
        }
        // remove from graph if it has not been already removed
        auto iter = ases->find(as->asn);
        if (iter != ases->end()) {
            ases->erase(as->asn);
        }
    }
    querier->clear_stubs_from_db();
    save_stubs_to_db(querier);
    querier->clear_non_stubs_from_db();
    save_non_stubs_to_db(querier);
}


/** Saves the stub ASes to be removed to a table on the database.
 *
 * @param querier
 */
void ASGraph::save_stubs_to_db(SQLQuerier *querier){
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir){
        mkdir("/dev/shm/bgp",0777);
    }
    else{
        closedir(dir);
    }

    std::ofstream outfile;
    std::cerr << "Saving Stubs..." << std::endl;
    std::string file_name = "/dev/shm/bgp/stubs.csv";
    outfile.open(file_name);
    for (auto &stub : *stubs_to_parents){
        outfile << stub.first << "," << stub.second << "\n";
    }
    outfile.close();
    querier->copy_stubs_to_db(file_name);
    std::remove(file_name.c_str());
}


/** Generate a csv with all supernodes, then dump them to database.
 *
 * @param querier
 */
void ASGraph::save_supernodes_to_db(SQLQuerier *querier) {
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir) {
        mkdir("/dev/shm/bgp",0777);
    } else {
        closedir(dir);
    }

    std::ofstream outfile;
    std::cerr << "Saving Supernodes..." << std::endl;
    std::string file_name = "/dev/shm/bgp/supernodes.csv";
    outfile.open(file_name);

    // Iterate over each strongly connected component
    for (auto &cur_node : *components) {
        if (cur_node->size() >= 2) {
            // Find the lowest asn in supernode
            uint32_t low = UINT_MAX;
            for (auto &cur_asn : *cur_node) {
                if (cur_asn < low)
                    low = cur_asn;
            }
            // assemble rows as pairs; asn in supernode, lowest asn in that supernode
            for (auto &cur_asn : *cur_node) {
                // outfile << cur_asn << "," << low << "," << ases->find(cur_asn)->second->rank << "\n";
                outfile << cur_asn << "," << low << "\n";
            }
        }
    }
    outfile.close();
    querier->copy_supernodes_to_db(file_name);
    std::cerr << "Saving Supernodes completed." << std::endl;
    //std::remove(file_name.c_str());
}


/** Saves the non_stub ASes to a table on the database.
 *
 * @param querier
 */
void ASGraph::save_non_stubs_to_db(SQLQuerier *querier){
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir){
        mkdir("/dev/shm/bgp",0777);
    }
    else{
        closedir(dir);
    }

    std::ofstream outfile;
    std::cerr << "Saving Non-Stubs..." << std::endl;
    std::string file_name = "/dev/shm/bgp/non-stubs.csv";
    outfile.open(file_name);
    for (auto non_stub : *non_stubs){
        outfile << non_stub << "\n";
    }
    outfile.close();
    querier->copy_non_stubs_to_db(file_name);
    std::remove(file_name.c_str());
}


/** Decide and assign ranks to all the AS's in the graph.
 *
 *  The rank of an AS is the maximum number of nodes it has below it. This means
 *  it is possible to have an AS of rank 0 directly below an AS of rank 4, but
 *  not possible to have an AS of rank 3 below one of rank 2.
 *
 *  The bottom of the DAG is rank 0.
 */
void ASGraph::decide_ranks() {
    // initial set of customer ASes at the bottom of the DAG
    ases_by_rank->push_back(new std::set<uint32_t>());
    // For ASes with no customers
    for (auto &as : *ases) {
        if (as.second->customers->empty()) {
            // if AS is a leaf node, or "stub" AS
            (*ases_by_rank)[0]->insert(as.first);
            as.second->rank = 0;
        }
    }
    //std::cerr << "Started rank 0" << std::endl;

    int i = 0;
    while (!(*ases_by_rank)[i]->empty()) {
        //std::cerr << "iter " << i << " size " << (*ases_by_rank)[i]->size() << std::endl;
        ases_by_rank->push_back(new std::set<uint32_t>());
        for (uint32_t asn : *(*ases_by_rank)[i]) {
            //For all providers of this AS
            for (const uint32_t &provider_asn : *ases->find(asn)->second->providers) {
                AS* prov_AS = ases->find(translate_asn(provider_asn))->second;
                int oldrank = prov_AS->rank;
                if (oldrank < i + 1) {
                    prov_AS->rank = i + 1;
                    (*ases_by_rank)[i+1]->insert(provider_asn);
                    if (oldrank != -1) {
                        (*ases_by_rank)[oldrank]->erase(provider_asn);
                    }
                }
            }
        }
        //std::cerr << "Completed rank " << i << std::endl;
        i++;
    }
    return;
}

/** Tarjan driver to detect strongly connected components in the ASGraph.
 *
 * https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
 */
void ASGraph::tarjan() {
    int index = 0;
    std::stack<AS*> s;
    int count = 0;
    int uc_count = 0;

    for (auto &as : *ases) {
        if (as.second->index == -1){
            tarjan_helper(as.second, index, s);
        }
    }
    return;
}

/** Tarjan algorithm to detect strongly connected components in the ASGraph.
 */
void ASGraph::tarjan_helper(AS *as, int &index, std::stack<AS*> &s) {
    as->index = index;
    as->lowlink = index;
    index++;
    s.push(as);
    as->onStack = true;

    for (auto &neighbor : *(as->providers)) {
        AS *n = ases->find(neighbor)->second;
        if (n->index == -1){
            tarjan_helper(n, index, s);
            as->lowlink = std::min(as->lowlink, n->lowlink);
        } else if (n->onStack) {
            as->lowlink = std::min(as->lowlink, n->index);
        }
    }
    if (as->lowlink == as->index) {
        std::vector<uint32_t> *component = new std::vector<uint32_t>;
        AS *as_from_stack;
        do{
            as_from_stack = s.top();
            s.pop();
            as_from_stack->onStack = false;
            component->push_back(as_from_stack->asn);
        } while (as_from_stack != as);
        components->push_back(component);
        //if (component->size() > 1)
        //    std::cerr << "Root node found: " << as->asn << " of size " << component->size() << std::endl;
    }
}


/** Combine providers, peers, and customers of ASes in a strongly connected component.
 *  Also append to component_translation for future reference.
 */
void ASGraph::combine_components(){

    // For each strongly connected component
    for (auto const& component : *components){
        // Ignore single AS nodes
        if (component->size() <= 1) {
            continue;
        }

        // Find indentifying ASN for supernode
        uint32_t combined_asn = component->at(0);
        for (auto &cur_asn : *component) {
            if (cur_asn < combined_asn) {
                combined_asn = cur_asn;
            }
        }

        // Combined Component will id as lowest ASN
        AS *combined_AS = new AS(combined_asn, inverse_results);

        // For all members of a component, gather neighbors
        for (auto &cur_asn : *component) {
            combined_AS->member_ases->push_back(cur_asn);
            // Get the AS object associated to ASN
            auto asn_search = ases->find(cur_asn);
            AS *cur_AS = asn_search->second;

            // Handle providers
            for (auto &provider_asn : *cur_AS->providers) {
                // Check if provider is in component
                bool external = (std::find(component->begin(),
                                           component->end(),
                                           provider_asn) == component->end());
                if (external) {
                    AS *provider_AS = ases->find(provider_asn)->second;
                    // Add new relationship
                    combined_AS->add_neighbor(provider_asn, AS_REL_PROVIDER);
                    provider_AS->add_neighbor(combined_asn, AS_REL_CUSTOMER);
                    // Handle overlapping peer, remove peer relationship from supernode
                    combined_AS->remove_neighbor(provider_asn, AS_REL_PEER);
                    // Remove old subnode customer relationship from external provider
                    provider_AS->remove_neighbor(cur_asn, AS_REL_CUSTOMER);
                    // Remove subnode peer relationship from external provider if it exists
                    provider_AS->remove_neighbor(cur_asn, AS_REL_PEER);
                }
            }

            // Handle customers
            for (auto &customer_asn : *cur_AS->customers) {
                // Check if customer is in component
                bool external = (std::find(component->begin(),
                                           component->end(),
                                           customer_asn) == component->end());
                if (external) {
                    AS *customer_AS = ases->find(customer_asn)->second;
                    // Add new relationship
                    combined_AS->add_neighbor(customer_asn, AS_REL_CUSTOMER);
                    customer_AS->add_neighbor(combined_asn, AS_REL_PROVIDER);
                    // Handle overlapping peer, remove redundant peer relationship from supernode
                    combined_AS->remove_neighbor(customer_asn, AS_REL_PEER);
                    // Remove old subnode provider relationship from external provider
                    customer_AS->remove_neighbor(cur_asn, AS_REL_PROVIDER);
                    // Remove redundant subnode peer relationship from external provider if it exists
                    customer_AS->remove_neighbor(cur_asn, AS_REL_PEER);
                }
            }

            // Handle peers
            for (auto &peer_asn: *cur_AS->peers){
                // Check if peer is in component
                bool external = (std::find(component->begin(),
                                      component->end(),
                                      peer_asn) == component->end());
                // Check if peer is already a provider in the combined AS
                bool no_provider_rel = (combined_AS->providers->find(peer_asn) ==
                                        combined_AS->providers->end());
                // Check if peer is already a customer in the combined AS
                bool no_customer_rel = (combined_AS->customers->find(peer_asn) ==
                                        combined_AS->customers->end());

                // Peer is safe to add to combined AS
                if (external && no_provider_rel && no_customer_rel) {
                    // DEBUG
                    if (ases->find(peer_asn) == ases->end()) {
                        std::cout << cur_AS->asn << " peer missing, " << peer_asn << std::endl;
                        continue;
                    }
                    AS *peer_AS = ases->find(peer_asn)->second;
                    // Add new relationship
                    combined_AS->add_neighbor(peer_asn, AS_REL_PEER);
                    peer_AS->add_neighbor(combined_asn, AS_REL_PEER);
                    // Remove old peer relation to the subnode
                    peer_AS->remove_neighbor(cur_asn, AS_REL_PEER);
                } else if (external) {
                    // Other relationship superseeds peer
                    AS *peer_AS = ases->find(peer_asn)->second;
                    // Remove the subnode peer relationship from the external node
                    peer_AS->remove_neighbor(cur_asn, AS_REL_PEER);
                }
            }
            // Append ASN translation and discard member
            component_translation->insert(std::pair<uint32_t, uint32_t>(cur_asn,combined_asn));
            ases->erase(cur_AS->asn);
            delete cur_AS;
        }
        // Insert complete combined node to ases
        //std::cout << *combined_AS;
        ases->insert(std::pair<uint32_t,AS*>(combined_asn,combined_AS));
    }
    return;
}


/** Clear all announcements in AS.
 */
void ASGraph::clear_announcements(){
    for (auto const& as : *ases){
        as.second->clear_announcements();
    }
    for (auto const& i : *inverse_results) {
        delete i.second;
    }
    inverse_results->clear();
}


/** Print all ASes for debug.
 */
void ASGraph::printDebug() {
    for (auto const& as : *ases) {
        std::cout << as.first << ':' << as.second->asn << std::endl;
    }
    return;
}


/** Output python code for making graphviz digraph.
 */
void ASGraph::to_graphviz(std::ostream &os) {
    std::string id = "";
    for (auto const &as : *ases) {
        os << "dot.node('" << as.second->asn << "', '" << as.second->asn << "')" << std::endl;
        for (auto customer : *as.second->customers) {
            os << "dot.edge('" << as.second->asn << "', '" << customer << "')" << std::endl;
        }
    }
}


/** Operation for debug printing AS
 */
std::ostream& operator<<(std::ostream &os, const ASGraph& asg) {
    os << "AS's" << std::endl;
    for (auto const& as : *(asg.ases)) {
        os << *as.second << std::endl;
    }
    return os;
}
