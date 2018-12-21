#include <cstdint>
#include <iostream>
#include <set>
#include <vector>
#include <stack>
#include <algorithm>
#include "ASGraph.h"
#include "AS.h"

ASGraph::ASGraph() {
    ases = new std::map<uint32_t, AS*>;    
    ases_by_rank = new std::vector<std::set<uint32_t>*>(255);
    components = new std::vector<std::vector<uint32_t>*>;
    component_translation = new std::map<uint32_t, uint32_t>;
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

    delete component_translation;
}

/** Adds an AS relationship to the graph.
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
        ases->insert(std::pair<uint32_t, AS*>(asn, new AS(asn)));
        search = ases->find(asn);
    }
    search->second->add_neighbor(neighbor_asn, relation);
}

//translates asn to asn of component it belongs to in graph
//returns 0 if asn isn't found
uint32_t ASGraph::translate_asn(uint32_t asn){
    auto search = component_translation->find(asn);
    if(search == component_translation->end()){
       return 0; 
    }
    return search->second;
}

void ASGraph::process(){
    tarjan();
    combine_components();
    decide_ranks();
    return;
}

void ASGraph::create_graph_from_files(){
    std::ifstream file;
    //TODO make file names part of config
    file.open("../peers.csv");
    if (file.is_open()){
        std::string line;
        while(getline(file, line)){
            line.erase(remove(line.begin(),line.end(),' '),line.end());
            if(line.empty() || line[0] == '#' || line[0] == '['){
                continue;
            }
            auto delim_index = line.find(",");
            std::string peer_as_1 = line.substr(0,delim_index);
            std::string peer_as_2 = line.substr(delim_index+1);
            add_relationship(std::stoi(peer_as_1), std::stoi(peer_as_2),1);
            add_relationship(std::stoi(peer_as_2), std::stoi(peer_as_1),1);
        }
    }
    file.close();
    file.clear();

    file.open("../customer_provider_pairs.csv");
    if (file.is_open()){
        std::string line;
        while(getline(file, line)){
            line.erase(remove(line.begin(),line.end(),' '),line.end());
            if(line.empty() || line[0] == '#' || line[0] == '['){
                continue;
            }
            auto delim_index = line.find(",");
            std::string customer_as = line.substr(0,delim_index);
            std::string provider_as = line.substr(delim_index+1);
            add_relationship(std::stoi(customer_as), std::stoi(provider_as),0);
            add_relationship(std::stoi(provider_as), std::stoi(customer_as),2);
        }
    }
    file.close();
    file.clear();

    //process strongly connected components, decide ranks
    process();

    return;   
}

void ASGraph::create_graph_from_db(SQLQuerier *querier){
    //TODO add table names to config
    pqxx::result R = querier->select_from_table("peers");
    //c[1] = peer_as_1, c[2] = peer_as_2
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        add_relationship(c[1].as<uint32_t>(),c[2].as<uint32_t>(),AS_REL_PEER);
        add_relationship(c[2].as<uint32_t>(),c[1].as<uint32_t>(),AS_REL_PEER);
    }
    //c[1] = customer_as, c[2] = provider_as
    R = querier->select_from_table("customer_provider_pairs");
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        add_relationship(c[1].as<uint32_t>(),c[2].as<uint32_t>(),AS_REL_PROVIDER);
        add_relationship(c[2].as<uint32_t>(),c[1].as<uint32_t>(),AS_REL_CUSTOMER);
    }

    process();

    return;
}


/** Decide and assign ranks to all the AS's in the graph. 
 */
void ASGraph::decide_ranks() {
    // initialize the sets
    for (int i = 0; i < 255; i++) {
        (*ases_by_rank)[i] = new std::set<uint32_t>();
    }
    // initial set of customer ASes at the bottom of the DAG
    for (auto &as : *ases) {
        if (as.second->customers->empty()) {
            // if AS is a leaf node, or "stub" AS
            (*ases_by_rank)[0]->insert(as.first);
            as.second->rank = 0;
        }
    }
    
    // this is decreased from 1000 to 254 because the TTL on an IPv4 packet
    // is at most 255, so in theory this is a reasonable maximum for ASes too.
    for (int i = 0; i < 254; i++) {
        if ((*ases_by_rank)[i]->empty()) {
            // if we are above the top of the DAG, stop
            return;
        }
        for (uint32_t asn : *(*ases_by_rank)[i]) {
            for (const uint32_t &provider_asn : *ases->find(asn)->second->providers) {
                auto prov_AS = ases->find(provider_asn)->second;
                if (prov_AS->rank == -1) {
                    int skip_provider = 0;
                    // TODO improve variable naming here
                    for (auto prov_cust_asn : *prov_AS->customers) {
                        auto *prov_cust_AS = ases->find(prov_cust_asn)->second;
                        if (prov_cust_AS->rank == -1 ||
                            prov_cust_AS->rank > i) {
                            skip_provider = 1;
                            break;
                        }
                    if(skip_provider){
                        continue;
                    }
                    ases->find(provider_asn)->second->rank = i + 1;
                    (*ases_by_rank)[i+1]->insert(provider_asn);
                    }
                }
            }
        }
    }
    return;
}

//https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
void ASGraph::tarjan() {
    int index = 0;
    std::stack<AS*> s;

    for (auto &as : *ases) {
        if (as.second->index == -1){
            tarjan_helper(as.second, index, s);
        }
    }
    return;
}

void ASGraph::tarjan_helper(AS *as, int &index, std::stack<AS*> &s) {
    as->index = index;
    as->lowlink = index;
    index++;
    s.push(as);
    as->onStack = true;

    for (auto &neighbor : *(as->providers)){
        AS *n = ases->find(neighbor)->second;
        if (n->index == -1){
            tarjan_helper(n, index, s);
            as->lowlink = std::min(as->lowlink,n->lowlink);
        }
        else if (n->onStack){
            as->lowlink = std::min(as->lowlink, n->index);
        }
    }
    if (as->lowlink == as->index){
        std::vector<uint32_t> *component = new std::vector<uint32_t>;
        AS *as_from_stack;
        do{
            as_from_stack = s.top();
            s.pop();
            as_from_stack->onStack = false;
            component->push_back(as_from_stack->asn);
        } while (as_from_stack != as);
        components->push_back(component);
    }
}

//Combine providers, peers, and customers of ASes in a strongly connected
//component. Also append to component_translation for future reference.
void ASGraph::combine_components(){
    for (auto const& component : *components){
        uint32_t combined_asn = component->at(0);
        AS *combined_AS = new AS(combined_asn);

        //For all members of component, gather neighbors
        for (unsigned int i = 0; i < component->size(); i++){
            uint32_t asn = component->at(i);
            auto search = ases->find(asn);
            AS *as = search->second;
            //Get providers
            for(uint32_t const provider_asn: *as->providers){
                //make sure provider isn't in this component
                if(std::find(component->begin(),component->end(), provider_asn)
                        == component->end())
                {
                    combined_AS->providers->insert(provider_asn);
                    //replace old customer of provider
                    AS *provider_AS = ases->find(provider_asn)->second;
                    provider_AS->customers->erase(asn);
                    provider_AS->customers->insert(combined_asn);
                }
            }
            //Get peers
            for(uint32_t const peer_asn: *as->peers){
                //make sure peer isn't in this component
                if(std::find(component->begin(),component->end(), peer_asn)
                        == component->end())
                {
                    combined_AS->peers->insert(peer_asn);
                    //replace old customer of provider
                    AS *peer_AS = ases->find(peer_asn)->second;
                    peer_AS->peers->erase(asn);
                    peer_AS->peers->insert(combined_asn);
                }
            }
            //Get customers
            for(uint32_t const customer_asn: *as->customers){
                //check if customer isn't in this component
                if(std::find(component->begin(),component->end(), customer_asn)
                        == component->end())
                {
                    combined_AS->customers->insert(customer_asn);
                    //replace old customer of provider
                    AS *customer_AS = ases->find(customer_asn)->second;
                    customer_AS->providers->erase(asn);
                    customer_AS->providers->insert(combined_asn);
                }
            }
            //append ASN translation and discard member
            component_translation->insert(std::pair<uint32_t, uint32_t>(asn,combined_asn));
            delete as;
            ases->erase(search);
        }
        //insert complete combined node to ases
        ases->insert(std::pair<uint32_t,AS*>(combined_asn,combined_AS));
    }
    return;
}

void ASGraph::clear_announcements(){
    for (auto const& as : *ases){
        as.second->clear_announcements();
    }
}

// print all as's
void ASGraph::printDebug() {
    for (auto const& as : *ases) {
        std::cout << as.first << ':' << as.second->asn << std::endl;
    }
    return; 
}

std::ostream& operator<<(std::ostream &os, const ASGraph& asg) {
    os << "AS's" << std::endl;
    for (auto const& as : *(asg.ases)) {
        os << *as.second << std::endl;
    }
    return os;
}


