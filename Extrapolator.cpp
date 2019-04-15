#include <cmath>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <thread>
#include "Extrapolator.h"

Extrapolator::Extrapolator() {
    graph = new ASGraph;
    querier = new SQLQuerier;
    threads = new std::vector<std::thread>;
}

Extrapolator::Extrapolator(bool invert_results) : Extrapolator() {
    invert = invert_results;
}

Extrapolator::~Extrapolator(){
    delete graph;
    delete querier;
    delete threads;
}

void Extrapolator::perform_propagation(bool test, int iteration_size, int max_total){
    using namespace std;
   
    // make tmp directory if it does not exist
    bool exists = false;
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir){
        mkdir("/dev/shm/bgp", 0777); 
    } else {
        closedir(dir);
    }

    // Generate required tables
    querier->create_results_tbl();
    querier->create_inverse_results_tbl();
    querier->create_stubs_tbl();
    querier->create_non_stubs_tbl();
    querier->create_supernodes_tbl();
    
    // Generate the graph and populate the stubs & supernode tables
    graph->create_graph_from_db(querier);
    
    //Get ROAs from "roas" table (prefix - origin pairs)
    std::cout << "Selecting prefixes with ROAs..." << std::endl;
    pqxx::result prefixes = querier->select_roa_prefixes(ROAS_TABLE, IPV4);

    int row_to_start_group = 0;
    int row_in_group = 0;
    int num_prefixes = prefixes.size();
    int iteration_num = 0;

    //Continue if not exeeding user defined or record max
    std::cout << "Beginning propagation..." << std::endl;
    while(row_to_start_group  < max_total && row_to_start_group  < num_prefixes){
        
        std::cerr << "On iteration number " << std::to_string(iteration_num) <<std::endl;
        row_in_group = 0;

        //NEW THREAD CAN START HERE

        std::vector<std::string> prefixes_to_get;

        //For prefixes in group such that user defined or record max isn't exceeded,
        //and iteration size isn't exceeded. 
        //  Add prefix to vector to use
        //TODO probably check if prefixes ever get reused in this loop
        for (pqxx::result::size_type i = 0 + row_to_start_group;
            i !=prefixes.size() && i - row_to_start_group < (unsigned)iteration_size &&
            row_to_start_group < max_total; ++i){
            //Skip ipv6, possible future support
            int ip_family;
            prefixes[i]["family"].to(ip_family);
            if(ip_family == 6)
                continue;
            //TODO remove redundent
            //std::string prefix_to_get = prefixes[i]["roa_prefix"].as<std::string>(); 
            prefixes_to_get.push_back(prefixes[i]["prefix"].as<std::string>());
        }
        row_to_start_group = iteration_num * iteration_size;
        std::cerr << "Generated Prefixes to Get" << std::endl;
        

        std::cerr << "Selecting Announcements..." << std::endl;
        //Get all announcements (R) for prefixes in iteration (prefixes_to_get)
        pqxx::result R = querier->select_ann_records(ANNOUNCEMENTS_TABLE,prefixes_to_get);
        std::cerr << "Done." << std::endl;

        std::cerr << "Parsing path vectors..." << std::endl;
        //For all returned announcements
        for (pqxx::result::size_type j = 0; j!=R.size(); ++j){
            std::string s = R[j]["host"].c_str();
            
            Prefix<> p(R[j]["host"].c_str(),R[j]["netmask"].c_str()); 
            uint32_t origin;
            R[j]["origin"].to(origin);
            auto po = std::pair<Prefix<>,uint32_t>(p,origin);
            // add this prefix to the inverse results
            if (graph->inverse_results->find(po) == graph->inverse_results->end()) {
                graph->inverse_results->insert(std::pair<std::pair<Prefix<>,uint32_t>,std::set<uint32_t>*>(
                    po, new std::set<uint32_t>()));
                // put all non-stub ASNs in the set
                for (uint32_t asn : *graph->non_stubs) {
                    graph->inverse_results->find(po)->second->insert(asn);
                }
            }

            //This bit of code parses array-like strings from db to get AS_PATH.
            //libpq++ doesn't currently support returning arrays.
            std::vector<uint32_t> *as_path = new std::vector<uint32_t>;
            std::string path_as_string(R[j]["as_path"].as<std::string>());
            //remove brackets from string
            char brackets[] = "{}";
            for (uint32_t i = 0; i < strlen(brackets); ++i){
                path_as_string.erase(std::remove(path_as_string.begin(),path_as_string.end(),
                                     brackets[i]), path_as_string.end()); 
            }
            //fill as_path vector from parsing string
            std::string delimiter = ",";
            int pos = 0;
            std::string token;
            while((pos = path_as_string.find(delimiter)) != std::string::npos){
                token = path_as_string.substr(0,pos);
                try {
                  as_path->push_back(std::stoul(token));
                } catch(const std::out_of_range& e) {
                  std::cerr << "Caught out of range error filling path vect, token was: " << token << std::endl; 
                } catch(const std::invalid_argument& e) {
                  std::cerr << "Invalid argument filling path vect, token was:"
                    << token << std::endl;
                }
                path_as_string.erase(0,pos + delimiter.length());
            }
            try {
              as_path->push_back(std::stoul(path_as_string));
            } catch(const std::out_of_range& e) {
              std::cerr << "Caught out of range error filling path vect (last), token was: " << token << std::endl; 
              std::cerr << "Path as string was: " << path_as_string << std::endl; 
            } catch(const std::invalid_argument& e) {
                  if (path_as_string.length() == 0) {
                    std::cerr << "Ignoring announcement with empty as_path" <<
                      std::endl;
                  } else {
                    std::cerr << "Invalid argument filling path vect (last), token was:"
                      << token << std::endl;
                    std::cerr << "Path as string was: " << path_as_string << std::endl;
                  }
            }
       
            //if no hop identify accordingly, otherwise use it
            // this "hop" field is never used and should be removed
            std::string hop;
            hop = "hop";
            give_ann_to_as_path(as_path,p,hop);
            delete as_path;
        }
        std::cerr << "Done." << std::endl;
        std::cerr << "Propagating..." << std::endl;
        //TODO send AS.anns_sent_to_peers_providers to rest of peers/providers
        propagate_up();
        propagate_down();
//        threads->push_back(std::thread(&Extrapolator::save_results,this,iteration_num));
        invert_results();
        save_results(iteration_num);
        graph->clear_announcements();
        row_in_group++;
        iteration_num++;
    }
    // create an index on the results
    querier->create_results_index();

    /*
    for (auto &t : *threads){
        t.join();
    }
    */
}


/** Propagate announcements from customers to peers and providers.
 */
void Extrapolator::propagate_up() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            graph->ases->find(asn)->second->process_announcements();
           // auto test = graph->ases->find(asn)->second;
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
    for (size_t level = levels-1; level-- > 0;) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            //std::cout << "propagating down to " << asn << std::endl;
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
    std::string hop) {
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
        uint32_t asn_on_path = graph->translate_asn(*it);
        AS *as_on_path = graph->ases->find(asn_on_path)->second;
            if (as_on_path->already_received(ann_to_check_for)) 
                continue;
        int sent_to = -1;
        // "If not at the most recent AS (rightmost in reversed path),
        // record the AS it is sent to next"
        //TODO maybe use iterator instead of i
        if (i < as_path->size() - 1) {
            auto asn_sent_to = *(it + 1);
            // This is refactored a little from the python code.
            // There is still probably a nicer way to do this.
            //TODO use relationship macros instead of 0,1,2
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
        // if ASes in the path aren't neighbors (If data is out of sync)
        bool broken_path = false;
        // now check recv'd from
        // It is 3 by default. It stays as 3 if it's the origin.
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
        //This is how priority is calculated
        double path_len_weighted = 1 - (i - 1) / 100;
        double priority = received_from + path_len_weighted;
       
        uint32_t received_from_asn = 0;
        //If this AS is the origin, it "received" the ann from itself
        if (it == as_path->rbegin()){
            uint32_t received_from_asn = *it;
        }
        else
            uint32_t received_from_asn = *(it-1);
        if (!broken_path) {
            Announcement ann = Announcement(
                *as_path->rbegin(),
                prefix.addr,
                prefix.netmask,
                priority,
                received_from_asn,
                hop,
                true); //from_monitor
            //If announcement was sent to a peer or provider
            //Append it to list of anns sent to peers and providers
            if (sent_to == 1 or sent_to == 0) {
                as_on_path->anns_sent_to_peers_providers->push_back(ann);
            }
            as_on_path->receive_announcement(ann);
        }
    }
}


//TODO remove this unused function
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
                    asn,
                    ann.second.hop));

            priority--; // subtract 1 for peers
            anns_to_peers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn,
                    ann.second.hop));
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
                    asn,
                    ann.second.hop));
        }
        // send announcements
        for (uint32_t customer_asn : *source_as->customers) {
            graph->ases->find(customer_asn)->second->receive_announcements(
                anns_to_customers);
        }
    }
}

/** Invert the extrapolation results for more compact storage. 
 *
 * Since a prefix is most often reachable from every AS in the internet, it
 * makes sense to store only the instances where an AS cannot reach a
 * particular prefix. In order to detect hijacks, we map distinct prefix-origin
 * pairs to sets of Autonomous Systems that have not selected a route to them.
 */
void Extrapolator::invert_results(void) {
    for (auto &as : *graph->ases) {
        for (auto ann : *as.second->all_anns) {
            // error checking?
            auto set = graph->inverse_results->find(
                std::pair<Prefix<>,uint32_t>(ann.second.prefix, ann.second.origin));
            if (set != graph->inverse_results->end()) {
                set->second->erase(as.second->asn);
            }
        }
    }
}

void Extrapolator::save_results(int iteration){
//    SQLQuerier *thread_querier = new SQLQuerier;
    std::ofstream outfile;
    std::string file_name = "/dev/shm/bgp/" + std::to_string(iteration) + ".csv";
    outfile.open(file_name);

    if (invert) {
        std::cerr << "Saving Inverse Results From Iteration: " << iteration << std::endl;
        for (auto po : *graph->inverse_results){
            for (uint32_t asn : *po.second) {
                outfile << asn << ","
                        << po.first.first.to_cidr() << ","
                        << po.first.second << std::endl;
            }
        }
        outfile.close();
        querier->copy_inverse_results_to_db(file_name);
    } else {
        std::cerr << "Saving Results From Iteration: " << iteration << std::endl;
        for (auto &as : *graph->ases){
            as.second->stream_announcements(outfile);
        }
        outfile.close();
        querier->copy_results_to_db(file_name);

    }
    std::remove(file_name.c_str());
}
