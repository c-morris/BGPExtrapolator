#include "Extrapolators/Extrapolator.h"

Extrapolator::Extrapolator(bool random /* = true */,
                 bool invert_results /* = true */, 
                 bool store_depref /* =false */, 
                 std::string a /* = ANNOUNCEMENTS_TABLE */,
                 std::string r /* = RESULTS_TABLE */, 
                 std::string i /* = INVERSE_RESULTS_TABLE */, 
                 std::string d /* = DEPREF_RESULTS_TABLE */, 
                 uint32_t iteration_size /* = false */) : BaseExtrapolator(random, invert_results, store_depref, a, r, i, d, iteration_size) {

    graph = new ASGraph();
    querier = new SQLQuerier(a, r, i, d);
}

void Extrapolator::perform_propagation() {
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

    querier->clear_stubs_from_db();
    querier->create_stubs_tbl();
    querier->clear_non_stubs_from_db();
    querier->create_non_stubs_tbl();
    querier->clear_supernodes_from_db();
    querier->create_supernodes_tbl();
    
    // Generate the graph and populate the stubs & supernode tables
    graph->create_graph_from_db(querier);
   
    std::cout << "Generating subnet blocks..." << std::endl;
    
    // Generate iteration blocks
    std::vector<Prefix<>*> *prefix_blocks = new std::vector<Prefix<>*>; // Prefix blocks
    std::vector<Prefix<>*> *subnet_blocks = new std::vector<Prefix<>*>; // Subnet blocks
    Prefix<> *cur_prefix = new Prefix<>("0.0.0.0", "0.0.0.0"); // Start at 0.0.0.0/0
    populate_blocks(cur_prefix, prefix_blocks, subnet_blocks); // Select blocks based on iteration size
    delete cur_prefix;
   
    std::cout << "Beginning propagation..." << std::endl;
    
    // Seed MRT announcements and propagate
    uint32_t announcement_count = 0; 
    int iteration = 0;
    auto ext_start = std::chrono::high_resolution_clock::now();
    
    // For each unprocessed prefix block
    extrapolate_blocks(announcement_count, iteration, false, prefix_blocks);
    
    // For each unprocessed subnet block  
    extrapolate_blocks(announcement_count, iteration, true, subnet_blocks);
    
    auto ext_finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> e = ext_finish - ext_start;
    std::cout << "Block elapsed time: " << e.count() << std::endl;
    
    // Cleanup
    delete prefix_blocks;
    delete subnet_blocks;
    
    std::cout << "Announcement count: " << announcement_count << std::endl;
}

void Extrapolator::extrapolate_blocks(uint32_t &announcement_count, 
                                      int &iteration, 
                                      bool subnet, 
                                      auto const& prefix_set) {
    // For each unprocessed block of announcements 
    for (Prefix<>* prefix : *prefix_set) {
        std::cout << "Selecting Announcements..." << std::endl;
        auto prefix_start = std::chrono::high_resolution_clock::now();
        
        // Handle prefix blocks or subnet blocks of announcements
        pqxx::result ann_block;
        if (!subnet) {
            // Get the block of announcements for the specific prefix
            ann_block = querier->select_prefix_ann(prefix);
        } else {
            // Get the block of announcements for the whole subnet
            ann_block = querier->select_subnet_ann(prefix);
        } 
        
        // Check for empty block
        auto bsize = ann_block.size();
        if (bsize == 0)
            break;
        announcement_count += bsize;
        
        std::cout << "Seeding announcements..." << std::endl;
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
            
            // Check for loops in the path and drop announcement if they exist
            bool loop = find_loop(as_path);
            if (loop) {
                static int g_loop = 1;
                
                Logger::getInstance().log("Loops") << "AS path loop #" << g_loop << ", Origin: " << origin << ", Prefix: " << cur_prefix.to_cidr() << ", Path: " << path_as_string;

                g_loop++;
                continue;
            }

            // Get timestamp
            int64_t timestamp = std::stol(ann_block[i]["time"].as<std::string>());

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
            // Seed announcements along AS path
            give_ann_to_as_path(as_path, cur_prefix, timestamp);
            delete as_path;
        }
        // Propagate for this subnet
        std::cout << "Propagating..." << std::endl;
        propagate_up();
        propagate_down();
        save_results(iteration);
        graph->clear_announcements();
        iteration++;
        
        std::cout << prefix->to_cidr() << " completed." << std::endl;
        auto prefix_finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> q = prefix_finish - prefix_start;
    }
}

void Extrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp) {
    // Handle empty as_path
    if (as_path->empty()) { 
        return;
    }
    
    uint32_t i = 0;
    uint32_t path_l = as_path->size();
    
    // Announcement at origin for checking along the path
    Announcement ann_to_check_for(as_path->at(path_l-1),
                                  prefix.addr,
                                  prefix.netmask,
                                  0,
                                  timestamp); 
    
    // Iterate through path starting at the origin
    for (auto it = as_path->rbegin(); it != as_path->rend(); ++it) {
        // Increments path length, including prepending
        i++;
        // If ASN not in graph, continue
        if (graph->ases->find(*it) == graph->ases->end()) {
            continue;
        }
        // Translate ASN to it's supernode
        uint32_t asn_on_path = graph->translate_asn(*it);
        // Find the current AS on the path
        AS *as_on_path = graph->ases->find(asn_on_path)->second;
        // Check if already received this prefix
        if (as_on_path->already_received(ann_to_check_for)) {
            // Find the already received announcement
            auto search = as_on_path->all_anns->find(ann_to_check_for.prefix);
            // If the current timestamp is newer (worse)
            if (ann_to_check_for.tstamp > search->second.tstamp) {
                // Skip it
                continue;
            } else if (ann_to_check_for.tstamp == search->second.tstamp) {
                // Tie breaker for equal timestamp
                bool value = true;
                // Random tiebreak if enabled
                if (random == true) {
                    value = as_on_path->get_random();
                }

                // Log annoucements with equal timestamps 
                Logger::getInstance().log("Equal_Timestamp") << "Equal Timestamp on announcements. Prefix: " << ann_to_check_for.prefix.to_cidr() << 
                    ", rand value: " << value << ", tstamp on announcements: " << ann_to_check_for.tstamp << 
                    ", origin on ann_to_check_for: " << ann_to_check_for.origin << ", origin on stored announcement: " << search->second.origin;

                // First come, first saved if random is disabled
                if (value) {
                    continue;
                } else {
                    // Position of previous AS on path
                    uint32_t pos = path_l - i + 1;
                    // Prepending check, use original priority
                    if (pos < path_l && as_path->at(pos) == as_on_path->asn) {
                        continue;
                    }
                    as_on_path->delete_ann(ann_to_check_for);
                }
            } else {
                // Log announcements that arent handled by sorting
                Logger::getInstance().log("Unsorted_Announcements") 
                    << "This announcement is being deleted and is not handled by sorting." 
                    << " Prefix: " << ann_to_check_for.prefix.to_cidr() 
                    << ", tstamp: " << ann_to_check_for.tstamp 
                    << ", origin: " << ann_to_check_for.origin;

                // Delete worse MRT announcement, proceed with seeding
                as_on_path->delete_ann(ann_to_check_for);
            }
        }
        
        // If ASes in the path aren't neighbors (data is out of sync)
        bool broken_path = false;

        // It is 3 by default. It stays as 3 if it's the origin.
        int received_from = 300;
        // If this is not the origin AS
        if (i > 1) {
            // Get the previous ASes relationship to current AS
            if (as_on_path->providers->find(*(it - 1)) != as_on_path->providers->end()) {
                received_from = AS_REL_PROVIDER;
            } else if (as_on_path->peers->find(*(it - 1)) != as_on_path->peers->end()) {
                received_from = AS_REL_PEER;
            } else if (as_on_path->customers->find(*(it - 1)) != as_on_path->customers->end()) {
                received_from = AS_REL_CUSTOMER;
            } else {
                broken_path = true;
            }
        }

        // This is how priority is calculated
        uint32_t path_len_weighted = 100 - (i - 1);
        uint32_t priority = received_from + path_len_weighted;
       
        uint32_t received_from_asn = 0;
        // If this AS is the origin
        if (it == as_path->rbegin()){
            // It received the ann from itself
            received_from_asn = *it;
        } else {
            // Otherwise received it from previous AS
            received_from_asn = *(it-1);
        }
        // No break in path so send the announcement
        if (!broken_path) {
            Announcement ann = Announcement(*as_path->rbegin(),
                                            prefix.addr,
                                            prefix.netmask,
                                            priority,
                                            received_from_asn,
                                            timestamp,
                                            true);
            // Send the announcement to the current AS
            as_on_path->process_announcement(ann, random);
            if (graph->inverse_results != NULL) {
                auto set = graph->inverse_results->find(
                        std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
                // Remove the AS from the prefix's inverse results
                if (set != graph->inverse_results->end()) {
                    set->second->erase(as_on_path->asn);
                }
            }
        } else {
            // Report the broken path
            //std::cerr << "Broken path for " << *(it - 1) << ", " << *it << std::endl;
            
            static int g_broken_path = 0;

            // Log the part of path where break takes place
            Logger::getInstance().log("Broken_Paths") << "Broken Path #" << g_broken_path << ", between these two ASes: " << *(it - 1) << ", " << *it;

            g_broken_path++;
        }
    }
}

/** Send all announcements kept by an AS to its neighbors. 
 *
 * This approximates the Adj-RIBs-out. 
 *
 * @param asn AS that is sending out announces
 * @param to_providers Send to providers
 * @param to_peers Send to peers
 * @param to_customers Send to customers
 */
void Extrapolator::send_all_announcements(uint32_t asn, 
                                          bool to_providers, 
                                          bool to_peers, 
                                          bool to_customers) {
    // Get the AS that is sending it's announcements
    auto *source_as = graph->ases->find(asn)->second; 
    // If we are sending to providers
    if (to_providers) {
        // Assemble the list of announcements to send to providers
        std::vector<Announcement> anns_to_providers;
        for (auto &ann : *source_as->all_anns) {
            // Do not propagate any announcements from peers/providers
            // Priority is reduced by 1 per path length
            // Base priority is 200 for customer to provider
            // Ignore announcements not from a customer
            if (ann.second.priority < 200) {
                continue;
            }
            
            // Set the priority of the announcement at destination 
            uint32_t old_priority = ann.second.priority;
            uint32_t path_len_weight = old_priority % 100;
            if (path_len_weight == 0) {
                // For MRT ann at origin: old_priority = 400
                path_len_weight = 99;
            } else {
                // Sub 1 for the current hop
                path_len_weight -= 1;
            }
            uint32_t priority = 200 + path_len_weight;
            
            // Push announcement with new priority to ann vector
            anns_to_providers.push_back(Announcement(ann.second.origin,
                                                     ann.second.prefix.addr,
                                                     ann.second.prefix.netmask,
                                                     priority,
                                                     asn,
                                                     ann.second.tstamp));
        }
        // Send the vector of assembled announcements
        for (uint32_t provider_asn : *source_as->providers) {
            // For each provider, give the vector of announcements
            auto *recving_as = graph->ases->find(provider_asn)->second;
            recving_as->receive_announcements(anns_to_providers);
        }
    }

    // If we are sending to peers
    if (to_peers) {
        // Assemble vector of announcement to send to peers
        std::vector<Announcement> anns_to_peers;
        for (auto &ann : *source_as->all_anns) {
            // Do not propagate any announcements from peers/providers
            // Priority is reduced by 1 per path length
            // Base priority is 100 for peers to peers

            // Ignore announcements not from a customer
            if (ann.second.priority < 200) {
                continue;
            }

            // Set the priority of the announcement at destination 
            uint32_t old_priority = ann.second.priority;
            uint32_t path_len_weight = old_priority % 100;
            if (path_len_weight == 0) {
                // For MRT ann at origin: old_priority = 400
                path_len_weight = 99;
            } else {
                // Sub 1 for the current hop
                path_len_weight -= 1;
            }
            uint32_t priority = 100 + path_len_weight;
            
            anns_to_peers.push_back(Announcement(ann.second.origin,
                                                 ann.second.prefix.addr,
                                                 ann.second.prefix.netmask,
                                                 priority,
                                                 asn,
                                                 ann.second.tstamp));
        }
        // Send the vector of assembled announcements
        for (uint32_t peer_asn : *source_as->peers) {
            // For each provider, give the vector of announcements
            auto *recving_as = graph->ases->find(peer_asn)->second;
            recving_as->receive_announcements(anns_to_peers);
        }
    }

    // If we are sending to customers
    if (to_customers) {
        // Assemble the vector of announcement for customers
        std::vector<Announcement> anns_to_customers;
        for (auto &ann : *source_as->all_anns) {
            // Propagate all announcements to customers
            // Priority is reduced by 1 per path length
            // Base priority is 0 for provider to customers
            
            
            // Set the priority of the announcement at destination 
            uint32_t old_priority = ann.second.priority;
            uint32_t path_len_weight = old_priority % 100;
            if (path_len_weight == 0) {
                // For MRT ann at origin: old_priority = 400
                path_len_weight = 99;
            } else {
                // Sub 1 for the current hop
                path_len_weight -= 1;
            }
            uint32_t priority = path_len_weight;

            anns_to_customers.push_back(Announcement(ann.second.origin,
                                                     ann.second.prefix.addr,
                                                     ann.second.prefix.netmask,
                                                     priority,
                                                     asn,
                                                     ann.second.tstamp));
        }
        // Send the vector of assembled announcements
        for (uint32_t customer_asn : *source_as->customers) {
            // For each customer, give the vector of announcements
            auto *recving_as = graph->ases->find(customer_asn)->second;
            recving_as->receive_announcements(anns_to_customers);
        }
    }
}