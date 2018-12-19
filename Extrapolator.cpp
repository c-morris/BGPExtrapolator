#include <cmath>
#include "Extrapolator.h"

Extrapolator::Extrapolator() {
    ases_with_anns = new std::set<uint32_t>;
    graph = new ASGraph;
    querier = new SQLQuerier;
    graph->create_graph_from_db(querier);
}

Extrapolator::~Extrapolator(){
    delete ases_with_anns;
    delete graph;
    delete querier;
}

void Extrapolator::perform_propagation(bool test, int iteration_size, int max_total){
    using namespace std;
    //TODO use better folder naming convention
    if(!test){
        if(mkdir("results", 0777)==-1)
            cerr << "Error: "<<  strerror(errno) <<endl;
        else
            cout << "Created \"test\" directory" <<endl;
    }
    pqxx::result prefixes = querier->select_distinct_prefixes_from_table("elements");
    
    int row_to_start_it = 0;
    int row_in_it = 0;
    for (pqxx::result::size_type i = 0 + row_to_start_it;
        i !=prefixes.size() && row_to_start_it + row_in_it < max_total; ++i){
        //TODO add support for ipv6
        int ip_family;
        if(prefixes[i]["family"].to(ip_family) == 6)
            continue;

        row_in_it = 0;    
        std::string prefix_to_get = prefixes[i]["prefix"].c_str();
        //TODO check num of announcements for one prefix with real dataset
        pqxx::result R = querier->select_ann_records("simplified_elements",prefix_to_get,1000);
        for (pqxx::result::size_type j = 0;
            j!=R.size() && row_in_it < iteration_size && 
            row_to_start_it + row_in_it < max_total; ++j){
        
            //TODO put this record prep/parsing into separate function
            //the next few blocks covert the ip strings from db to uint_32        
    //        pqxx::result anns = querier->select_ann_records("elements",it[0].as<std::string>());


            std::string s = R[j]["host"].c_str();
            std::cerr << s << std::endl;
            
            Prefix<> p(R[j]["host"].c_str(),R[j]["netmask"].c_str()); 

            //This bit of code parses array-like strings from db to get AS_PATH.
            //libpq++ doesn't currently support returning arrays.
            std::vector<uint32_t> *as_path = new std::vector<uint32_t>;
            std::string path_as_string = R[j]["as_path"].c_str();
            //remove brackets from string
            char brackets[] = "{}";
            for (int i = 0; i < strlen(brackets); ++i){
                path_as_string.erase(std::remove(path_as_string.begin(),path_as_string.end(),
                                     brackets[i]), path_as_string.end()); 
            }
            //fill as_path vector from parsing string
            std::string delimiter = ",";
            int pos = 0;
            std::string token;
            while((pos = path_as_string.find(delimiter)) != std::string::npos){
                token = path_as_string.substr(0,pos);
                as_path->push_back(std::stoi(token));
                path_as_string.erase(0,pos + delimiter.length());
            }
            as_path->push_back(std::stoi(path_as_string));
        
            uint32_t hop;
            R[j]["next_hop"].to(hop);
            give_ann_to_as_path(as_path,p,hop);
     
            row_in_it++;
        }
        row_to_start_it++;
    }
    propagate_up();
    propagate_down();
    //TODO count iteration num
    save_results(1);
}


/** Propagate announcements from customers to peers and providers.
 */
void Extrapolator::propagate_up() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            graph->ases->find(asn)->second->process_announcements();
            if (!graph->ases->find(asn)->second->all_anns->empty()) {
                send_all_announcements(asn, true, false);
            }
        }
    }
}

/** Send "best" announces to customer ASes. 
 */
void Extrapolator::propagate_down() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = levels-1; level > 0; level--) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            graph->ases->find(asn)->second->process_announcements();
            if (!graph->ases->find(asn)->second->all_anns->empty()) {
                send_all_announcements(asn, false, true);
            }
        }
    }
}

/** Record announcement on all ASes on as_path. 
 *
 * @param as_path List of ASes for this announcement.
 * @param prefix The prefix this announcement is for.
 * @param hop The first ASN on the as_path.
 */
void Extrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, 
    Prefix<> prefix,
    uint32_t hop) {
    // handle empty as_path
    if (as_path->empty()) 
        return;
    Announcement ann_to_check_for(as_path->at(as_path->size()-1),
                                  prefix.addr,
                                  prefix.netmask,
                                  0); // invalid from_asn?
    // i is used to traverse as_path
    uint32_t i = 0; 
    // iterate backwards through path
    for (auto it = as_path->rbegin(); it != as_path->rend(); ++it) {
        i++;
        // if asn not in graph, continue
        if (graph->ases->find(*it) == graph->ases->end()) 
            continue;
        // TODO some SCC stuff here
        // comp_id = self.graph.ases[asn].SCC_id
//        uint32_t comp_id = *it;
        uint32_t asn_on_path = graph->translate_asn(*it);
        AS *as_on_path = graph->ases->find(asn_on_path)->second;
        // if (comp_id in self.ases_with_anns)
            if (as_on_path->already_received(ann_to_check_for)) 
                continue;
        int sent_to = -1;
        // "If not at the most recent AS (rightmost in reversed path),
        // record the AS it is sent to next"
        if (i < as_path->size() - 1) {
            auto asn_sent_to = *(it + 1);
            // This is refactored a little from the python code.
            // There is still probably a nicer way to do this.
            //if (asn_sent_to not in strongly connected components[comp_id]) { 
                if (as_on_path->providers->find(asn_sent_to) !=
                    as_on_path->providers->end()) {
                    sent_to = 0;
                } else if (as_on_path->peers->find(asn_sent_to) !=
                    as_on_path->peers->end()) {
                    sent_to = 1;
                } else if (as_on_path->customers->find(asn_sent_to) != 
                    as_on_path->customers->end()) {
                    sent_to = 2;
                }
        }
        //}
        // if ASes in the path aren't neighbors (this happens sometimes)
        bool broken_path = false;
        // now check recv'd from
        // TODO Do we need to prefer announces from customers here? because
        // right now it looks like we prefer providers
        int received_from = 3;
        if (i > 1) {
            if (as_on_path->providers->find(*(it - 1)) != 
                as_on_path->providers->end()) {
                received_from = 0;
            } else if (as_on_path->peers->find(*(it - 1)) != 
                as_on_path->providers->end()) {
                received_from = 1;
            } else if (as_on_path->customers->find(*(it - 1)) !=
                as_on_path->providers->end()) {
                received_from = 2;
            } else {
                broken_path = true;
            }
        }
        double path_len_weighted = 1 - (i - 1) / 100;
        double priority = received_from + path_len_weighted;
        if (!broken_path) {
            Announcement ann = Announcement(
                *as_path->rbegin(),
                prefix.addr,
                prefix.netmask,
                priority,
                *(it - 1),
                true);
            if (sent_to == 1 or sent_to == 0) {
                as_on_path->anns_sent_to_peers_providers->push_back(ann);
            }
            as_on_path->receive_announcement(ann);
            ases_with_anns->insert(asn_on_path);
        }
    }
}

/** Query all announcements for a vector of prefixes from the database and
 * insert them into the graph. 
 *
 * @param prefixes a vector of prefixes to query the db for
 */
void Extrapolator::insert_announcements(std::vector<Prefix<>> *prefixes) {
    using namespace pqxx;
    // this is very db library dependent, so I'm leaving it for you, Michael
  //  result R = querier->select_ann_records("simplified_elements"
    return;
}

/** Send all announcements kept by an AS to its neighbors. 
 *
 * This approximates the Adj-RIBs-out. 
 *
 * @param asn AS that is sending out announces
 * @param to_peers_providers send to peers and providers
 * @param to_customers send to customers
 */
void Extrapolator::send_all_announcements(uint32_t asn, 
    bool to_peers_providers, 
    bool to_customers) {
    auto *source_as = graph->ases->find(asn)->second; 
    if (to_peers_providers) {
        std::vector<Announcement> anns_to_providers;
        std::vector<Announcement> anns_to_peers;
        for (auto &ann : *source_as->all_anns) {
            if (ann.second.priority < 2)
                continue;
            // priority is reduced by 0.01 for path length
            // base priority is 2 for providers
            // base priority is 1 for peers
            // do not propagate any announcements from peers
            double priority = ann.second.priority;
            if(priority - floor(priority) == 0)
                priority = 2.99;
            else
                priority = priority - floor(priority) - 0.01 + 2;
            anns_to_providers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn));

            priority--; // subtract 1 for peers
            anns_to_peers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn));
        }
        // send announcements
        for (uint32_t provider_asn : *source_as->providers) {
            graph->ases->find(provider_asn)->second->receive_announcements(
                anns_to_providers);
        }
        for (uint32_t peer_asn : *source_as->peers) {
            graph->ases->find(peer_asn)->second->receive_announcements(
                anns_to_peers);
        }
    }

    // now do the same for customers
    if (to_customers) {
        std::vector<Announcement> anns_to_customers;
        for (auto &ann : *source_as->all_anns) {
            // priority is reduced by 0.01 for path length
            // base priority is 0 for customers
            double priority = ann.second.priority;
            if(priority - floor(priority) == 0)
                priority = 0.99;
            else
                priority = priority - floor(priority) - 0.01;
            anns_to_customers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn));
        }
        // send announcements
        for (uint32_t customer_asn : *source_as->customers) {
            graph->ases->find(customer_asn)->second->receive_announcements(
                anns_to_customers);
        }
    }
}

void Extrapolator::save_results(int iteration){
    std::ofstream outfile;
    outfile.open("results/" + std::to_string(iteration) + ".csv");
    //TODO accomodate for components
    for (auto &as : *graph->ases){
        (*as.second).stream_announcements(outfile);
        outfile  << std::endl;
    }
    outfile.close();
}
