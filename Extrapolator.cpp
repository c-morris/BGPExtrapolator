#include <cmath>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <thread>
#include <chrono>
#include "Extrapolator.h"

Extrapolator::Extrapolator(bool invert_results, bool store_depref, std::string a, std::string r, 
        std::string i, std::string d, uint32_t iteration_size) {
    invert = invert_results;                    // True to store the results inverted
    depref = store_depref;                      // True to store the second best ann for depref
    it_size = iteration_size;                   // Number of prefix to be precessed per iteration
    graph = new ASGraph;
    threads = new std::vector<std::thread>;
    querier = new SQLQuerier(a, r, i, d);
}

Extrapolator::~Extrapolator(){
    delete graph;
    delete querier;
    delete threads;
}


/** Performs all tasks necessary to propagate a set of announcements given:
 *      1) A populated mrt_announcements table
 *      2) A populated customer_provider table
 *      3) A populated peers table
 *
 * @param test
 * @param iteration_size number of rows to process each iteration, rounded down to the nearest full prefix
 * @param max_total maximum number of rows to process, ignored if zero
 */
void Extrapolator::perform_propagation(bool test, size_t iteration_size, size_t max_total){
    using namespace std;
   
    // Make tmp directory if it does not exist
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir){
        mkdir("/dev/shm/bgp", 0777); 
    } else {
        closedir(dir);
    }

    // Generate required tables
    if (invert) {
        querier->clear_inverse_from_db();
        querier->create_inverse_results_tbl();
    } else {
        querier->clear_results_from_db();
        querier->create_results_tbl();
    }
    if (depref) {
        querier->clear_depref_from_db();
        querier->create_depref_tbl();
    }
    querier->create_stubs_tbl();
    querier->clear_stubs_from_db();
    querier->create_non_stubs_tbl();
    querier->clear_non_stubs_from_db();
    querier->create_supernodes_tbl();
    querier->clear_supernodes_from_db();
    
    // Generate the graph and populate the stubs & supernode tables
    graph->create_graph_from_db(querier);
   
    std::cout << "Generating subnet blocks..." << std::endl;
    
    // Generate iteration blocks
    std::vector<Prefix<>*> *prefix_blocks = new std::vector<Prefix<>*>; // Prefix blocks
    std::vector<Prefix<>*> *subnet_blocks = new std::vector<Prefix<>*>; // Subnet blocks
    Prefix<> *cur_prefix = new Prefix<>("0.0.0.0", "0.0.0.0"); // Start at 0.0.0.0/0
    populate_blocks(cur_prefix, prefix_blocks, subnet_blocks);
    delete cur_prefix;
   
    
    std::cout << "Beginning propagation..." << std::endl;
    uint32_t announcement_count = 0; 
    // For each unprocessed prefix block  
    int iteration = 0;
    double avg_it_time = 0;
    auto bloc_start = std::chrono::high_resolution_clock::now();
    for (Prefix<>* prefix : *prefix_blocks) {
        std::cerr << "Selecting Announcements..." << std::endl;
        
        auto prefix_start = std::chrono::high_resolution_clock::now();
        
        // Get the block of announcements for the subgroup
        pqxx::result ann_block = querier->select_prefix_ann(prefix); 
        // Check for empty block
        auto bsize = ann_block.size();
        if (bsize == 0)
            break;
        announcement_count += bsize;
        
        // For all announcements in this block
        for (pqxx::result::size_type i = 0; i < bsize; i++) {
            // Get row origin
            uint32_t origin;
            ann_block[i]["origin"].to(origin);
            // Get row prefix
            std::string ip = ann_block[i]["host"].c_str();
            std::string mask = ann_block[i]["netmask"].c_str();
            Prefix<> cur_prefix(ip, mask);
            // Get row AS path
            std::string path_as_string(ann_block[i]["as_path"].as<std::string>());
            std::vector<uint32_t> *as_path = parse_path(path_as_string);
            
            // Assemble pair
            auto prefix_origin = std::pair<Prefix<>, uint32_t>(cur_prefix, origin);
            
            // Insert the inverse results for this prefix
            if (graph->inverse_results->find(prefix_origin) == graph->inverse_results->end()) {
                // This is horrifying
                graph->inverse_results->insert(std::pair<std::pair<Prefix<>, uint32_t>, 
                                                        std::set<uint32_t>*>
                                                        (prefix_origin, new std::set<uint32_t>()));
                
                // Put all non-stub ASNs in the set
                for (uint32_t asn : *graph->non_stubs) {
                    graph->inverse_results->find(prefix_origin)->second->insert(asn);
                }
            }

            // Process AS path
            give_ann_to_as_path(as_path, cur_prefix);
            delete as_path;
        }
        // Propagate for this subnet
        propagate_up();
        propagate_down();
        save_results(iteration);
        graph->clear_announcements();
        iteration++;
        
        std::cout << prefix->to_cidr() << " completed." << std::endl;
        auto prefix_finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> q = prefix_finish - prefix_start;
        //std::cout << "Prefix process time: " << q.count() << std::endl;
        avg_it_time += q.count();
        avg_it_time /= 2;
    }
    // For each unprocessed subnet block  
    for (Prefix<>* subnet : *subnet_blocks) {
        std::cerr << "Selecting Announcements..." << std::endl;
        
        auto subnet_start = std::chrono::high_resolution_clock::now();
        
        // Get the block of announcements for the subgroup
        pqxx::result ann_block = querier->select_subnet_ann(subnet); 
        // Check for empty block
        auto bsize = ann_block.size();
        if (bsize == 0)
            break;
        announcement_count += bsize;
        
        // For all announcements in this block
        for (pqxx::result::size_type i = 0; i < bsize; i++) {
            // Get row origin
            uint32_t origin;
            ann_block[i]["origin"].to(origin);
            // Get row prefix
            std::string ip = ann_block[i]["host"].c_str();
            std::string mask = ann_block[i]["netmask"].c_str();
            Prefix<> cur_prefix(ip, mask);
            // Get row AS path
            std::string path_as_string(ann_block[i]["as_path"].as<std::string>());
            std::vector<uint32_t> *as_path = parse_path(path_as_string);
            
            // Assemble pair
            auto prefix_origin = std::pair<Prefix<>, uint32_t>(cur_prefix, origin);
            
            // Insert the inverse results for this prefix
            if (graph->inverse_results->find(prefix_origin) == graph->inverse_results->end()) {
                // This is horrifying
                graph->inverse_results->insert(std::pair<std::pair<Prefix<>, uint32_t>, 
                                                        std::set<uint32_t>*>
                                                        (prefix_origin, new std::set<uint32_t>()));
                
                // Put all non-stub ASNs in the set
                for (uint32_t asn : *graph->non_stubs) {
                    graph->inverse_results->find(prefix_origin)->second->insert(asn);
                }
            }

            // Process AS path
            give_ann_to_as_path(as_path, cur_prefix);
            delete as_path;
        }
        // Propagate for this subnet
        propagate_up();
        propagate_down();
        save_results(iteration);
        graph->clear_announcements();
        iteration++;
        
        std::cout << subnet->to_cidr() << " completed." << std::endl;
        auto subnet_finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> q = subnet_finish - subnet_start;
        //std::cout << "Subnet process time: " << q.count() << std::endl;
        
        avg_it_time += q.count();
        avg_it_time /= 2;
    }
    auto bloc_finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> e = bloc_finish - bloc_start;
    std::cout << "Block elapsed time: " << e.count() << std::endl;
    std::cout << "Avg. iteration time: " << avg_it_time << std::endl;
    delete prefix_blocks;
    delete subnet_blocks;
    
    std::cout << "Announcement count: " << announcement_count << std::endl;

    // Create an index on the results
    /**
    if (!invert) {
        querier->create_results_index();
    }
    */
}


/** Recursive function to break the input mrt_announcements into manageable blocks.
 *
 * @param p The current subnet for checking announcement block size
 * @param prefix_vector The vector of prefixes of appropriate size
 */
template <typename Integer>
void Extrapolator::populate_blocks(Prefix<Integer>* p,
                                   std::vector<Prefix<>*>* prefix_vector,
                                   std::vector<Prefix<>*>* bloc_vector) {
    
    // Find the number of announcements within the subnet
    pqxx::result r = querier->select_subnet_count(p);
    
    // DEBUG
    //std::cout << "Prefix: " << p->to_cidr() << std::endl;
    //std::cout << "Count: "<< r[0][0].as<uint32_t>() << std::endl;

    if (r[0][0].as<uint32_t>() < it_size) {
        // Add to subnet block vector
        if (r[0][0].as<uint32_t>() > 0) {
            Prefix<Integer>* p_copy = new Prefix<Integer>(p->addr, p->netmask);
            bloc_vector->push_back(p_copy);
        }
    } else {
        // Store the prefix if there are announcements for it
        pqxx::result r2 = querier->select_prefix_count(p);
        if (r2[0][0].as<uint32_t>() > 0) {
            Prefix<Integer>* p_copy = new Prefix<Integer>(p->addr, p->netmask);
            prefix_vector->push_back(p_copy);
        }

        // Split prefix
        // First half: increase the prefix length by 1
        Integer new_mask;
        if (p->netmask == 0) {
            new_mask = p->netmask | 0x80000000;
        } else {
            new_mask = (p->netmask >> 1) | p->netmask;
        }
        Prefix<Integer>* p1 = new Prefix<Integer>(p->addr, new_mask);
	
        // Second half: increase the prefix length by 1 and flip previous length bit
        int8_t sz = 0;
        Integer new_addr = p->addr;
        for (int i = 0; i < 32; i++) {
            if (p->netmask & (1 << i)) {
                sz++;
            }
        }
        new_addr |= 1UL << (32 - sz - 1);
        Prefix<Integer>* p2 = new Prefix<Integer>(new_addr, new_mask);

        // Recursive call on each new prefix subnet
        populate_blocks(p1, prefix_vector, bloc_vector);
        populate_blocks(p2, prefix_vector, bloc_vector);

        delete p1;
        delete p2;
    }
}


/** Parse array-like strings from db to get AS_PATH in a vector.
 *
 * libpqxx doesn't currently support returning arrays, so we need to do this. 
 * path_as_string is passed in as a copy on purpose, since otherwise it would
 * be destroyed.
 *
 * @param path_as_string the as_path as a string returned by libpqxx
 */
std::vector<uint32_t>* Extrapolator::parse_path(std::string path_as_string) {
    std::vector<uint32_t> *as_path = new std::vector<uint32_t>;
    // Remove brackets from string
    path_as_string.erase(std::find(path_as_string.begin(), path_as_string.end(), '}'));
    path_as_string.erase(std::find(path_as_string.begin(), path_as_string.end(), '{'));

    // Fill as_path vector from parsing string
    std::string delimiter = ",";
    std::string::size_type pos = 0;
    std::string token;
    while((pos = path_as_string.find(delimiter)) != std::string::npos){
        token = path_as_string.substr(0,pos);
        try {
            as_path->push_back(std::stoul(token));
        } catch(...) {
            std::cerr << "Parse path error, token was: " << token << std::endl;
        }
        path_as_string.erase(0,pos + delimiter.length());
    }

    try {
        as_path->push_back(std::stoul(path_as_string));
    } catch(...) {
        std::cerr << "Parse path error, token was: " << path_as_string << std::endl;
    }
    return as_path;
}


/** Propagate announcements from customers to peers and providers ASes.
 */
void Extrapolator::propagate_up() {
    size_t levels = graph->ases_by_rank->size();
    // Propagate to providers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            graph->ases->find(asn)->second->process_announcements();
           // auto test = graph->ases->find(asn)->second;
            if (!graph->ases->find(asn)->second->all_anns->empty()) {
                send_all_announcements(asn, true, false, false);
            }
        }
    }
    // Propagate to peers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            graph->ases->find(asn)->second->process_announcements();
           // auto test = graph->ases->find(asn)->second;
            if (!graph->ases->find(asn)->second->all_anns->empty()) {
                send_all_announcements(asn, false, true, false);
            }
        }
    }
}


/** Send "best" announces from providers to customer ASes. 
 */
void Extrapolator::propagate_down() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = levels-1; level-- > 0;) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            //std::cerr << "propagating down to " << asn << std::endl;
            graph->ases->find(asn)->second->process_announcements();
            if (!graph->ases->find(asn)->second->all_anns->empty()) {
                send_all_announcements(asn, false, false, true);
            }
        }
    }
}


/** Record announcement on all ASes on as_path. 
 *
 * The from_monitor attribute is set to true on these announcements so they are
 * not replaced later during propagation. 
 *
 * @param as_path Vector of ASNs for this announcement.
 * @param prefix The prefix this announcement is for.
 */
void Extrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix) {
    // Handle empty as_path
    if (as_path->empty()) 
        return;
    Announcement ann_to_check_for(as_path->at(as_path->size()-1),
                                  prefix.addr,
                                  prefix.netmask,
                                  0); // invalid from_asn? yeah it means the announcement originates here 
    uint32_t i = 0; 
    // Iterate through path starting at the monitor
    for (auto it = as_path->rbegin(); it != as_path->rend(); ++it) {
        i++;
        // TODO We're ignoring missing ASes
        // If ASN not in graph, continue
        if (graph->ases->find(*it) == graph->ases->end()) 
            continue;
        // Translate ASN to it's supernode
        uint32_t asn_on_path = graph->translate_asn(*it);
        // TODO Fix this
        AS *as_on_path = graph->ases->find(asn_on_path)->second;
        if (as_on_path->already_received(ann_to_check_for)) 
            continue;
        int sent_to = -1;
        // "If not at the most recent AS (rightmost in reversed path),
        // record the AS it is sent to next"
        //TODO maybe use iterator instead of i
        if (i < as_path->size() - 1) {
            auto asn_sent_to = *(it + 1);
            //TODO use relationship macros instead of 0,1,2
            if (as_on_path->providers->find(asn_sent_to) !=
                as_on_path->providers->end()) {
                sent_to = AS_REL_PROVIDER;
                //sent_to = 0;
            } else if (as_on_path->peers->find(asn_sent_to) !=
                       as_on_path->peers->end()) {
                sent_to = AS_REL_PEER;
                //sent_to = 1;
            } else if (as_on_path->customers->find(asn_sent_to) != 
                       as_on_path->customers->end()) {
                sent_to = AS_REL_CUSTOMER;
                //sent_to = 2;
            }
        }
        // TODO Never checked
        // If ASes in the path aren't neighbors (If data is out of sync)
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
            received_from_asn = *it;
        }
        else
            received_from_asn = *(it-1);
        if (!broken_path) {
            Announcement ann = Announcement(*as_path->rbegin(),
                                            prefix.addr,
                                            prefix.netmask,
                                            priority,
                                            received_from_asn,
                                            true); //from_monitor
            //If announcement was sent to a peer or provider
            //Append it to list of anns sent to peers and providers
            if (sent_to == 1 or sent_to == 0) {
                as_on_path->anns_sent_to_peers_providers->push_back(ann);
            }
            as_on_path->receive_announcement(ann);
            if (graph->inverse_results != NULL) {
                auto set = graph->inverse_results->find(
                    std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
                if (set != graph->inverse_results->end()) {
                    set->second->erase(as_on_path->asn);
                }
            }
        }
    }
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
                                          bool to_providers, 
                                          bool to_peers, 
                                          bool to_customers) {
    auto *source_as = graph->ases->find(asn)->second; 
    if (to_providers) {
        std::vector<Announcement> anns_to_providers;
        for (auto &ann : *source_as->all_anns) {
            if (ann.second.priority < 2) {
                continue;
            }
            // priority is reduced by 0.01 for path length
            // base priority is 2 for providers
            // base priority is 1 for peers
            // do not propagate any announcements from peers
            double priority = ann.second.priority;
            if(priority - floor(priority) == 0) {
                priority = 2.99;
            } else {
                priority = priority - floor(priority) - 0.01 + 2;
            }
            anns_to_providers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn));
        }
        // send announcements
        for (uint32_t provider_asn : *source_as->providers) {
            auto *recving_as = graph->ases->find(provider_asn)->second;
            recving_as->receive_announcements(anns_to_providers);
            //recving_as->process_announcements();
        }
    }

    if (to_peers) {
        std::vector<Announcement> anns_to_peers;
        
        for (auto &ann : *source_as->all_anns) {
            if (ann.second.priority < 2) {
                continue;
            }
            // priority is reduced by 0.01 for path length
            // base priority is 2 for providers
            // base priority is 1 for peers
            // do not propagate any announcements from peers
            double priority = ann.second.priority;
            if(priority - floor(priority) == 0) {
                priority = 2.99;
            } else {
                priority = priority - floor(priority) - 0.01 + 2;
            }

            priority--; // subtract 1 for peers
            anns_to_peers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn));
        }


        for (uint32_t peer_asn : *source_as->peers) {
            auto *recving_as = graph->ases->find(peer_asn)->second;
            recving_as->receive_announcements(anns_to_peers);
            //recving_as->process_announcements();
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
            auto *recving_as = graph->ases->find(customer_asn)->second;
            recving_as->receive_announcements(anns_to_customers);
            //recving_as->process_announcements();
        }
    }
}


// TODO Remove unused function? 
/** Invert the extrapolation results for more compact storage. 
 *
 * Since a prefix is most often reachable from every AS in the internet, it
 * makes sense to store only the instances where an AS cannot reach a
 * particular prefix. In order to detect hijacks, we map distinct prefix-origin
 * pairs to sets of Autonomous Systems that have not selected a route to them.
 *
 * NO LONGER USED, inversion is now done during propagation
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


/** Save the results of a single iteration to a in-memory
 *
 * @param iteration The current iteration of the propagation
 */
void Extrapolator::save_results(int iteration){
    std::ofstream outfile;
    std::string file_name = "/dev/shm/bgp/" + std::to_string(iteration) + ".csv";
    outfile.open(file_name);
    
    // Handle inverse results
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
    
    // Handle standard results
    } else {
        std::cerr << "Saving Results From Iteration: " << iteration << std::endl;
        for (auto &as : *graph->ases){
            as.second->stream_announcements(outfile);
        }
        outfile.close();
        querier->copy_results_to_db(file_name);

    }
    std::remove(file_name.c_str());
    
    // Handle depref results
    if (depref) {
        std::string depref_name = "/dev/shm/bgp/depref" + std::to_string(iteration) + ".csv";
        outfile.open(depref_name);
        std::cerr << "Saving Depref From Iteration: " << iteration << std::endl;
        for (auto &as : *graph->ases) {
            as.second->stream_depref(outfile);
        }
        outfile.close();
        querier->copy_depref_to_db(depref_name);
        std::remove(depref_name.c_str());
    }
}
