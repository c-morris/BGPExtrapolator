#include "Extrapolators/BlockedExtrapolator.h"


template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::~BlockedExtrapolator() {

}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
void BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::init() {
        // Make tmp directory if it does not exist
    DIR* dir = opendir("/dev/shm/bgp");
    if(!dir){
        mkdir("/dev/shm/bgp", 0777); 
    } else {
        closedir(dir);
    }

    // Generate required tables
    if (this->store_results) {
        this->querier->clear_results_from_db();
        this->querier->create_results_tbl();
    }

    if (this->store_invert_results) {
        this->querier->clear_inverse_from_db();
        this->querier->create_inverse_results_tbl();
    }

    if (this->store_depref_results) {
        this->querier->clear_depref_from_db();
        this->querier->create_depref_tbl();
    }

    if (this->full_path_asns != NULL) {
        this->querier->clear_full_path_from_db();
        this->querier->create_full_path_results_tbl();
    }

    this->querier->clear_stubs_from_db();
    this->querier->create_stubs_tbl();
    this->querier->clear_non_stubs_from_db();
    this->querier->create_non_stubs_tbl();
    this->querier->clear_supernodes_from_db();
    this->querier->create_supernodes_tbl();
    
    // Generate the graph and populate the stubs & supernode tables
    this->graph->create_graph_from_db(this->querier);
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
void BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::perform_propagation() {
    init();

    
    if (!select_block_id) {
        BOOST_LOG_TRIVIAL(info) << "Generating subnet blocks...";
        // Generate iteration blocks
        std::vector<Prefix<PrefixType>*> *prefix_blocks = new std::vector<Prefix<PrefixType>*>; // Prefix blocks
        std::vector<Prefix<PrefixType>*> *subnet_blocks = new std::vector<Prefix<PrefixType>*>; // Subnet blocks
        Prefix<PrefixType> *cur_prefix = new Prefix<PrefixType>("0.0.0.0", "0.0.0.0"); // Start at 0.0.0.0/0
        this->populate_blocks(cur_prefix, prefix_blocks, subnet_blocks); // Select blocks based on iteration size
        delete cur_prefix;
        
        extrapolate(prefix_blocks, subnet_blocks);
        // Cleanup
        delete prefix_blocks;
        delete subnet_blocks;
    } else { // If blocks are selected by block_id from the announcement table
        BOOST_LOG_TRIVIAL(info) << "Extrapolating blocks...";
        // Find the max block_id and save it
        pqxx::result r = this->querier->select_max_block_id();
        max_block_id = r[0][0].as<uint32_t>();

        this->extrapolate_by_block_id(max_block_id);
    }
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
void BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::extrapolate(std::vector<Prefix<PrefixType>*> *prefix_blocks, std::vector<Prefix<PrefixType>*> *subnet_blocks) {
    BOOST_LOG_TRIVIAL(info) << "Beginning propagation...";
    
    // Seed MRT announcements and propagate
    uint32_t announcement_count = 0; 
    int iteration = 0;
    auto ext_start = std::chrono::high_resolution_clock::now();
    
    // For each unprocessed prefix block
    this->extrapolate_blocks(announcement_count, iteration, false, prefix_blocks);
    
    // For each unprocessed subnet block  
    this->extrapolate_blocks(announcement_count, iteration, true, subnet_blocks);

    auto ext_finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> e = ext_finish - ext_start;
    BOOST_LOG_TRIVIAL(info) << "Block elapsed time: " << e.count();
    BOOST_LOG_TRIVIAL(info) << "Announcement count: " << announcement_count;
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
void BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::extrapolate_by_block_id(uint32_t max_block_id) { 
    BOOST_LOG_TRIVIAL(info) << "Beginning propagation...";
    
    // Seed MRT announcements and propagate
    uint32_t announcement_count = 0; 
    int iteration = 0;
    auto ext_start = std::chrono::high_resolution_clock::now();

    std::thread save_res_thread;

    // Propagate each unprocessed block of announcements 
    for (uint32_t i = 0; i <= max_block_id; i++) {
        BOOST_LOG_TRIVIAL(info) << "Selecting Announcements...";
        auto prefix_start = std::chrono::high_resolution_clock::now();

        // Get a block of announcements
        int address_family = (sizeof(PrefixType) == 4 ? 4 : 6);
        pqxx::result ann_block = this->querier->select_prefix_block_id(i, address_family);

        // Check for empty block
        auto bsize = ann_block.size();
        if (bsize == 0) {
            BOOST_LOG_TRIVIAL(info) << "No announcements with this block id...";
            continue;
        }
        announcement_count += bsize;

        BOOST_LOG_TRIVIAL(info) << "Seeding announcements...";

        // For all announcements in this block
        for (pqxx::result::size_type i = 0; i < bsize; i++) {
            // Get row origin
            uint32_t origin;
            ann_block[i]["origin"].to(origin);
            // Get row prefix
            std::string ip = ann_block[i]["host"].c_str();
            std::string mask = ann_block[i]["netmask"].c_str();
            Prefix<PrefixType> cur_prefix(ip, mask);
            // Get row AS path
            std::string path_as_string(ann_block[i]["as_path"].as<std::string>());
            std::vector<uint32_t> *as_path = this->parse_path(path_as_string);
            
            // Check for loops in the path and drop announcement if they exist
            bool loop = this->find_loop(as_path);
            if (loop) {
                continue;
            }

            // Get timestamp
            int64_t timestamp = std::stol(ann_block[i]["time"].as<std::string>());

            if(this->graph->inverse_results != NULL) {
                // Assemble pair
                auto prefix_origin = std::pair<Prefix<PrefixType>, uint32_t>(cur_prefix, origin);
                
                // Insert the inverse results for this prefix
                if (this->graph->inverse_results->find(prefix_origin) == this->graph->inverse_results->end()) {
                    // This is horrifying
                    this->graph->inverse_results->insert(std::pair<std::pair<Prefix<PrefixType>, uint32_t>, 
                                                            std::set<uint32_t>*>
                                                            (prefix_origin, new std::set<uint32_t>()));
                    
                    // Put all non-stub ASNs in the set
                    for (uint32_t asn : *this->graph->non_stubs) {
                        this->graph->inverse_results->find(prefix_origin)->second->insert(asn);
                    }
                }
            }

            uint32_t prefix_id = std::stol(ann_block[i]["prefix_id"].as<std::string>());

            // Seed announcements along AS path
            this->give_ann_to_as_path(as_path, cur_prefix, timestamp, prefix_id);
            delete as_path;
        }
        // Propagate for this subnet
        BOOST_LOG_TRIVIAL(info) << "Propagating...";
        this->propagate_up();
        this->propagate_down();

        // Make sure we finish saving to the database before running save_results() on the next prefix
        if (save_res_thread.joinable()) {
            save_res_thread.join();
        }

        // Run save_results() in a separate thread
        save_res_thread = std::thread(&BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::save_results, this, iteration);

        // Wait for all csvs to be saved before clearing the announcements
        for (int i = 0; i < this->max_workers; i++) {
            sem_wait(&this->csvs_written);
        }

        this->graph->clear_announcements();
        iteration++;
        
        BOOST_LOG_TRIVIAL(info) << "block_id " << i << " completed.";
        auto prefix_finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> q = prefix_finish - prefix_start;
    }
    
    // Finalize saving before exiting the function
    if (save_res_thread.joinable()) {
        save_res_thread.join();
    }

    auto ext_finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> e = ext_finish - ext_start;
    BOOST_LOG_TRIVIAL(info) << "Block elapsed time: " << e.count();
    BOOST_LOG_TRIVIAL(info) << "Announcement count: " << announcement_count;
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
void BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::populate_blocks(Prefix<PrefixType>* p,
                                                                            std::vector<Prefix<PrefixType>*>* prefix_vector,
                                                                            std::vector<Prefix<PrefixType>*>* bloc_vector) { 
    // Find the number of announcements within the subnet
    pqxx::result r = this->querier->select_subnet_count(p);

    /** DEBUG
    std::cout << "Prefix: " << p->to_cidr() << std::endl;
    std::cout << "Count: "<< r[0][0].as<int>() << std::endl;
    */
    
    // If the subnet count is within size constraint
    if (r[0][0].as<uint32_t>() < this->iteration_size) {
        // Add to subnet block vector
        if (r[0][0].as<uint32_t>() > 0) {
            Prefix<PrefixType>* p_copy = new Prefix<PrefixType>(p->addr, p->netmask);
            bloc_vector->push_back(p_copy);
        }
    } else {
        // Store the prefix if there are announcements for it specifically
        pqxx::result r2 = this->querier->select_prefix_count(p);
        if (r2[0][0].as<uint32_t>() > 0) {
            Prefix<PrefixType>* p_copy = new Prefix<PrefixType>(p->addr, p->netmask);
            prefix_vector->push_back(p_copy);
        }

        Prefix<PrefixType>* p1;
        Prefix<PrefixType>* p2;

        if (std::is_same<PrefixType, uint32_t>::value) {
            // Split prefix
            // First half: increase the prefix length by 1
            uint32_t new_mask;
            if (p->netmask == 0) {
                new_mask = p->netmask | 0x80000000;
            } else {
                new_mask = (p->netmask >> 1) | p->netmask;
            }
            p1 = new Prefix<PrefixType>(p->addr, new_mask);

            // Second half: increase the prefix length by 1 and flip previous length bit
            int8_t sz = 0;
            uint32_t new_addr = p->addr;
            for (int i = 0; i < 32; i++) {
                if (p->netmask & (1 << i)) {
                    sz++;
                }
            }
            new_addr |= 1UL << (32 - sz - 1);
            p2 = new Prefix<PrefixType>(new_addr, new_mask);
        } else {
            // Split prefix
            // First half: increase the prefix length by 1
            uint128_t new_mask;
            if (p->netmask == 0) {
                new_mask = p->netmask | ((uint128_t) 1 << 127);  // 0x80000000000000000000000000000
            } else {
                new_mask = (p->netmask >> 1) | p->netmask;
            }
            p1 = new Prefix<PrefixType>(p->addr, new_mask);

            // Second half: increase the prefix length by 1 and flip previous length bit
            int32_t sz = 0;
            uint128_t new_addr = p->addr;
            for (int i = 0; i < 128; i++) {
                if (p->netmask & ((uint128_t) 1 << i)) {
                    sz++;
                }
            }
            new_addr |= (uint128_t) 1 << (128 - sz - 1);;
            p2 = new Prefix<PrefixType>(new_addr, new_mask);
        }

        // Recursive call on each new prefix subnet
        populate_blocks(p1, prefix_vector, bloc_vector);
        populate_blocks(p2, prefix_vector, bloc_vector);

        delete p1;
        delete p2;
    }
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
void BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::extrapolate_blocks(uint32_t &announcement_count, 
                                                                                                    int &iteration, 
                                                                                                    bool subnet, 
                                                                                                    std::vector<Prefix<PrefixType>*> *prefix_set) {
    std::thread save_res_thread;
    
    // For each unprocessed block of announcements 
    for (Prefix<PrefixType>* prefix : *prefix_set) {
        BOOST_LOG_TRIVIAL(info) << "Selecting Announcements...";
        auto prefix_start = std::chrono::high_resolution_clock::now();
        
        // Handle prefix blocks or subnet blocks of announcements
        pqxx::result ann_block;
        if (!subnet) {
            // Get the block of announcements for the specific prefix
            ann_block = this->querier->select_prefix_ann(prefix);
        } else {
            // Get the block of announcements for the whole subnet
            ann_block = this->querier->select_subnet_ann(prefix);
        } 
        
        // Check for empty block
        auto bsize = ann_block.size();
        if (bsize == 0)
            break;
        announcement_count += bsize;
        
        BOOST_LOG_TRIVIAL(info) << "Seeding announcements...";
        // For all announcements in this block
        for (pqxx::result::size_type i = 0; i < bsize; i++) {
            // Get row origin
            uint32_t origin;
            ann_block[i]["origin"].to(origin);
            // Get row prefix
            std::string ip = ann_block[i]["host"].c_str();
            std::string mask = ann_block[i]["netmask"].c_str();
            Prefix<PrefixType> cur_prefix(ip, mask);
            // Get row AS path
            std::string path_as_string(ann_block[i]["as_path"].as<std::string>());
            std::vector<uint32_t> *as_path = this->parse_path(path_as_string);
            
            // Check for loops in the path and drop announcement if they exist
            bool loop = this->find_loop(as_path);
            if (loop) {
                static int g_loop = 1;
                
                // Logger::getInstance().log("Loops") << "AS path loop #" << g_loop << ", Origin: " << origin << ", Prefix: " << cur_prefix.to_cidr() << ", Path: " << path_as_string;

                g_loop++;
                continue;
            }

            // Get timestamp
            int64_t timestamp = std::stol(ann_block[i]["time"].as<std::string>());

            if(this->graph->inverse_results != NULL) {
                // Assemble pair
                auto prefix_origin = std::pair<Prefix<PrefixType>, uint32_t>(cur_prefix, origin);
                
                // Insert the inverse results for this prefix
                if (this->graph->inverse_results->find(prefix_origin) == this->graph->inverse_results->end()) {
                    // This is horrifying
                    this->graph->inverse_results->insert(std::pair<std::pair<Prefix<PrefixType>, uint32_t>, 
                                                            std::set<uint32_t>*>
                                                            (prefix_origin, new std::set<uint32_t>()));
                    
                    // Put all non-stub ASNs in the set
                    for (uint32_t asn : *this->graph->non_stubs) {
                        this->graph->inverse_results->find(prefix_origin)->second->insert(asn);
                    }
                }
            }

            uint32_t prefix_id = std::stol(ann_block[i]["prefix_id"].as<std::string>());

            // Seed announcements along AS path
            this->give_ann_to_as_path(as_path, cur_prefix, timestamp, prefix_id);
            delete as_path;
        }
        // Propagate for this subnet
        BOOST_LOG_TRIVIAL(info) << "Propagating...";
        this->propagate_up();
        this->propagate_down();

        // Make sure we finish saving to the database before running save_results() on the next prefix
        if (save_res_thread.joinable()) {
            save_res_thread.join();
        }

        // Run save_results() in a separate thread
        save_res_thread = std::thread(&BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>::save_results, this, iteration);

        // Wait for all csvs to be saved before clearing the announcements
        for (int i = 0; i < this->max_workers; i++) {
            sem_wait(&this->csvs_written);
        }

        this->graph->clear_announcements();
        iteration++;
        
        BOOST_LOG_TRIVIAL(info) << prefix->to_cidr() << " completed.";
        auto prefix_finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> q = prefix_finish - prefix_start;
    }

    // Finalize saving before exiting the function
    if (save_res_thread.joinable()) {
        save_res_thread.join();
    }
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
void BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<PrefixType> prefix, int64_t timestamp, uint32_t prefix_id) {
    // Handle empty as_path
    if (as_path->empty()) { 
        return;
    }
    
    uint32_t i = 0;
    uint32_t path_l = as_path->size();
    
    // Announcement at origin for checking along the path
    // AnnouncementType ann_to_check_for(as_path->at(path_l-1),
    //                                     prefix.addr,
    //                                     prefix.netmask,
    //                                     0,
    //                                     timestamp); 
    
    // Iterate through path starting at the origin
    for (auto it = as_path->rbegin(); it != as_path->rend(); ++it) {
        // Only seed at origin AS if origin only mode is enabled
        if (this->origin_only == true && it != as_path->rbegin()) {
            return;
        }

        // Increments path length, including prepending
        i++;
        // If ASN not in graph, continue
        if (this->graph->ases->find(*it) == this->graph->ases->end()) {
            continue;
        }
        // Translate ASN to it's supernode
        uint32_t asn_on_path = this->graph->translate_asn(*it);
        // Find the current AS on the path
        ASType *as_on_path = this->graph->ases->find(asn_on_path)->second;

        auto announcement_search = as_on_path->all_anns->find(prefix);

        // If ASes in the path aren't neighbors (data is out of sync)
        bool broken_path = false;

        // It is 3 by default. It stays as 3 if it's the origin.
        int received_from = 3;
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
        Priority priority;
        priority.path_length = i - 1;
        priority.relationship = received_from;

        // Check if already received this prefix
        if (announcement_search != as_on_path->all_anns->end()) {
            AnnouncementType& second_announcement = announcement_search->second;

            // If the current timestamp is newer (worse)
            if (timestamp > second_announcement.tstamp) {
                // Skip it
                continue;
            } else if (timestamp == second_announcement.tstamp) {
                // Position of previous AS on path
                uint32_t prevPos = path_l - i + 1;

                // Position of the current AS on path
                uint32_t currPos = path_l - i;

                // Tie breaker for equal timestamp
                bool keep_first = true;
                // Random tiebreak if enabled
                if (this->random_tiebraking) {
                    keep_first = as_on_path->get_random();
                }

                // Skip announcement if there exists one with a higher priority
                ASType *current_as = this->graph->ases->find(as_path->at(currPos))->second;
                if (current_as->all_anns->find(prefix)->second.priority > priority) {
                    continue;
                // If the new announcement has a higher priority, change keep_first to false to make sure we save it
                } else if (current_as->all_anns->find(prefix)->second.priority < priority) {
                    keep_first = false;
                }

                // Log annoucements with equal timestamps 
                // Logger::getInstance().log("Equal_Timestamp") << "Equal Timestamp on announcements. Prefix: " << prefix.to_cidr() << 
                //     ", rand value: " << keep_first << ", tstamp on announcements: " << timestamp << 
                //     ", origin on ann_to_check_for: " << as_path->at(path_l-1) << ", origin on stored announcement: " << second_announcement.origin;

                // First come, first saved if random is disabled
                if (keep_first) {
                    continue;
                } else {
                    // Prepending check, use original priority
                    if (prevPos < path_l && prevPos >= 0 && as_path->at(prevPos) == as_on_path->asn) {
                        continue;
                    }
                    as_on_path->delete_ann(prefix);
                }
            } else {
                // Log announcements that arent handled by sorting
                // Logger::getInstance().log("Unsorted_Announcements") 
                //     << "This announcement is being deleted and is not handled by sorting." 
                //     << " Prefix: " << prefix.to_cidr() 
                //     << ", tstamp: " << timestamp 
                //     << ", origin: " << as_path->at(path_l-1);

                // Delete worse MRT announcement, proceed with seeding
                as_on_path->delete_ann(prefix);
            }
        }
        
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
            AnnouncementType ann = AnnouncementType(*as_path->rbegin(),
                                                    prefix.addr,
                                                    prefix.netmask,
                                                    priority,
                                                    received_from_asn,
                                                    timestamp,
                                                    true,
                                                    prefix_id);
            // Send the announcement to the current AS
            as_on_path->process_announcement(ann, this->random_tiebraking);
            if (this->graph->inverse_results != NULL) {
                auto set = this->graph->inverse_results->find(
                        std::pair<Prefix<PrefixType>,uint32_t>(ann.prefix, ann.origin));
                // Remove the AS from the prefix's inverse results
                if (set != this->graph->inverse_results->end()) {
                    set->second->erase(as_on_path->asn);
                }
            }
        } else {
            // Report the broken path
            //std::cerr << "Broken path for " << *(it - 1) << ", " << *it << std::endl;
            
            static int g_broken_path = 0;

            // Log the part of path where break takes place
            // Logger::getInstance().log("Broken_Paths") << "Broken Path #" << g_broken_path << ", between these two ASes: " << *(it - 1) << ", " << *it;

            g_broken_path++;
        }
    }
}

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType>
void BlockedExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType, PrefixType>::send_all_announcements(uint32_t asn, 
                                                                                                        bool to_providers, 
                                                                                                        bool to_peers, 
                                                                                                        bool to_customers) {
    // Get the AS that is sending it's announcements
    auto *source_as = this->graph->ases->find(asn)->second;

    // If to_customers = true and the AS is multihomed, return now for efficiency
    if (mh_mode == 1) {
        // Check if AS is multihomed
        if (source_as->customers->empty()) {
            if (to_customers) {
                return;
            }
        }
    }

    // Don't propagate from multihomed
    if (mh_mode == 2) {
        // Check if AS is multihomed
        if (source_as->customers->empty()) {
            return;
        }
    }

    // Only propagate to peers from multihomed
    if (mh_mode == 3) {
            // Check if AS is multihomed
            if (source_as->customers->empty()) {
                if (to_peers) {
                    to_providers = false;
                    to_customers = false;
                } else {
                    return;
                }
                
            }
    }

    // If we are sending to providers
    if (to_providers) {
        // Assemble the list of announcements to send to providers
        std::vector<AnnouncementType> anns_to_providers;
        for (auto &ann : *source_as->all_anns) {
            // Do not propagate any announcements from peers/providers
            // Priority is reduced by 1 per path length
            // Base priority is 2 for customer to provider
            // Ignore announcements not from a customer

            if (ann.second.priority.relationship < 2) { 
                continue;
            }

            // Automatic multihomed mode
            // Propagate an announcement to providers if none received an announcement from that prefix with origin = current asn
            uint32_t providers_with_ann = 0;
            if (mh_mode == 1) {
                // Check if AS is multihomed
                if (source_as->customers->empty()) {
                    // Check if all providers have the announcement
                    for (uint32_t provider_asn : *source_as->providers) {
                        auto *recving_as = this->graph->ases->find(provider_asn)->second;
                        auto search = recving_as->all_anns->find(ann.second.prefix);
                        if (search != recving_as->all_anns->end() && search->second.origin == ann.second.origin) {
                            providers_with_ann++;
                            break; // Break because at least one provider received the announcement from this AS
                        }
                    }
                }
            } 
            
            // If no providers have the announcement, propagate to providers
            if (providers_with_ann == 0) {
                // Set the priority of the announcement at destination 
                Priority priority;
                priority.relationship = 2;
                priority.path_length = ann.second.priority.path_length + 1;
                
                AnnouncementType temp = AnnouncementType(ann.second);
                temp.priority = priority;
                temp.from_monitor = false;
                temp.received_from_asn = asn;

                // Push announcement with new priority to ann vector
                // anns_to_providers.push_back(AnnouncementType(ann.second.origin,
                //                                          ann.second.prefix.addr,
                //                                          ann.second.prefix.netmask,
                //                                          priority,
                //                                          asn,
                //                                          ann.second.tstamp));

                anns_to_providers.push_back(temp);
            }
        }
        // Send the vector of assembled announcements
        for (uint32_t provider_asn : *source_as->providers) {
            // For each provider, give the vector of announcements
            auto *recving_as = this->graph->ases->find(provider_asn)->second;
            recving_as->receive_announcements(anns_to_providers);
        }
    }

    // If we are sending to peers
    if (to_peers) {
        // Assemble vector of announcement to send to peers
        std::vector<AnnouncementType> anns_to_peers;
        for (auto &ann : *source_as->all_anns) {
            // Do not propagate any announcements from peers/providers
            // Priority is reduced by 1 per path length
            // Base priority is 100 for peers to peers

            // Ignore announcements not from a customer
            if (ann.second.priority.relationship < 2) {
                continue;
            }

            Priority priority;
            priority.relationship = 1;
            priority.path_length = ann.second.priority.path_length + 1;
            
            // anns_to_peers.push_back(AnnouncementType(ann.second.origin,
            //                                             ann.second.prefix.addr,
            //                                             ann.second.prefix.netmask,
            //                                             priority,
            //                                             asn,
            //                                             ann.second.tstamp));

            AnnouncementType temp = AnnouncementType(ann.second);
            temp.priority = priority;
            temp.from_monitor = false;
            temp.received_from_asn = asn;

            anns_to_peers.push_back(temp);
        }
        // Send the vector of assembled announcements
        for (uint32_t peer_asn : *source_as->peers) {
            // For each provider, give the vector of announcements
            auto *recving_as = this->graph->ases->find(peer_asn)->second;
            recving_as->receive_announcements(anns_to_peers);
        }
    }

    // If we are sending to customers
    if (to_customers) {
        // Assemble the vector of announcement for customers
        std::vector<AnnouncementType> anns_to_customers;
        for (auto &ann : *source_as->all_anns) {
            // Propagate all announcements to customers
            // Priority is reduced by 1 per path length
            // Base priority is 0 for provider to customers
            
            
            // Set the priority of the announcement at destination 
            Priority priority;
            priority.path_length = ann.second.priority.path_length + 1;

            // anns_to_customers.push_back(AnnouncementType(ann.second.origin,
            //                                                 ann.second.prefix.addr,
            //                                                 ann.second.prefix.netmask,
            //                                                 priority,
            //                                                 asn,
            //                                                 ann.second.tstamp));

            //Use the copy constructor so that the inherited copy constructor will be called as well
            AnnouncementType temp = AnnouncementType(ann.second);
            temp.priority = priority;
            temp.from_monitor = false;
            temp.received_from_asn = asn;

            anns_to_customers.push_back(temp);
        }
        // Send the vector of assembled announcements
        for (uint32_t customer_asn : *source_as->customers) {
            // For each customer, give the vector of announcements
            auto *recving_as = this->graph->ases->find(customer_asn)->second;
            recving_as->receive_announcements(anns_to_customers);
        }
    }
}

template class BlockedExtrapolator<SQLQuerier<>, ASGraph<>, Announcement<>, AS<>, uint32_t>;
template class BlockedExtrapolator<SQLQuerier<uint128_t>, ASGraph<uint128_t>, Announcement<uint128_t>, AS<uint128_t>, uint128_t>;
template class BlockedExtrapolator<EZSQLQuerier, EZASGraph, EZAnnouncement, EZAS, uint32_t>;
template class BlockedExtrapolator<ROVSQLQuerier, ROVASGraph, ROVAnnouncement, ROVAS>;
