#include "Extrapolators/ROVExtrapolator.h"

ROVExtrapolator::ROVExtrapolator(bool random_tiebraking,
                                bool store_results, 
                                std::vector<std::string> policy_tables,
                                std::string announcement_table,
                                std::string results_table, 
                                std::string full_path_results_table, 
                                std::string config_section,
                                uint32_t iteration_size,
                                int exclude_as_number,
                                uint32_t mh_mode,
                                bool origin_only,
                                std::vector<uint32_t> *full_path_asns,
                                int max_threads) : BlockedExtrapolator(random_tiebraking, store_results, false, false, 
                                iteration_size, mh_mode, origin_only, full_path_asns, max_threads) {
    
    graph = new ROVASGraph(store_invert_results, store_depref_results);
    querier = new ROVSQLQuerier(policy_tables, announcement_table, results_table, full_path_results_table, exclude_as_number, config_section);
}

ROVExtrapolator::ROVExtrapolator() 
    : ROVExtrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, std::vector<std::string>(),
                        ANNOUNCEMENTS_TABLE, RESULTS_TABLE, FULL_PATH_RESULTS_TABLE, DEFAULT_QUERIER_CONFIG_SECTION, 
                        DEFAULT_ITERATION_SIZE, -1, DEFAULT_MH_MODE, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS) { }

ROVExtrapolator::~ROVExtrapolator() { }

void ROVExtrapolator::extrapolate_blocks(uint32_t &announcement_count, int &iteration, bool subnet, std::vector<Prefix<>*> *prefix_set) {
    // For each unprocessed block of announcements 
    for (Prefix<>* prefix : *prefix_set) {
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
            Prefix<> cur_prefix(ip, mask);
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

            // Get validity of an announcement
            int32_t roa_validity = std::stol(ann_block[i]["roa_validity"].as<std::string>());

            if(this->graph->inverse_results != NULL) {
                // Assemble pair
                auto prefix_origin = std::pair<Prefix<>, uint32_t>(cur_prefix, origin);
                
                // Insert the inverse results for this prefix
                if (this->graph->inverse_results->find(prefix_origin) == this->graph->inverse_results->end()) {
                    // This is horrifying
                    this->graph->inverse_results->insert(std::pair<std::pair<Prefix<>, uint32_t>, 
                                                            std::set<uint32_t>*>
                                                            (prefix_origin, new std::set<uint32_t>()));
                    
                    // Put all non-stub ASNs in the set
                    for (uint32_t asn : *this->graph->non_stubs) {
                        this->graph->inverse_results->find(prefix_origin)->second->insert(asn);
                    }
                }
            }

            // Seed announcements along AS path
            this->give_ann_to_as_path(as_path, cur_prefix, timestamp, roa_validity);
            delete as_path;
        }
        // Propagate for this subnet
        BOOST_LOG_TRIVIAL(info) << "Propagating...";
        this->propagate_up();
        this->propagate_down();
        this->save_results(iteration);
        this->graph->clear_announcements();
        iteration++;
        
        BOOST_LOG_TRIVIAL(info) << prefix->to_cidr() << " completed.";
        auto prefix_finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> q = prefix_finish - prefix_start;
    }
}

void ROVExtrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp /* = 0 */, uint32_t roa_validity /* = 1 */) {
    // Handle empty as_path
    if (as_path->empty()) { 
        return;
    }
    
    uint32_t i = 0;
    uint32_t path_l = as_path->size();
    
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
        ROVAS *as_on_path = this->graph->ases->find(asn_on_path)->second;

        auto announcement_search = as_on_path->all_anns->find(prefix);

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

        // Check if already received this prefix
        if (announcement_search != as_on_path->all_anns->end()) {
            ROVAnnouncement& second_announcement = announcement_search->second;

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
                ROVAS *current_as = this->graph->ases->find(as_path->at(currPos))->second;
                if (current_as->all_anns->find(prefix)->second.priority > priority) {
                    continue;
                // If the new announcement has a higher priority, change keep_first to false to make sure we save it
                } else if (current_as->all_anns->find(prefix)->second.priority < priority) {
                    keep_first = false;
                }

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
                // Delete worse MRT announcement, proceed with seeding
                as_on_path->delete_ann(prefix);
            }
        }
        
        uint32_t received_from_asn = 0;

        // ROV Handle origin received_from here
        // HIJACKED = 64513
        // NOTHIJACKED = 64514
        
        // If this AS is the origin
        if (it == as_path->rbegin()){
            // ROV: Set the received_from_asn to proper flag
            if (roa_validity != 0 && roa_validity != 1) {
                received_from_asn = HIJACKED_ASN;
                // Mark this AS as an attacker
                this->graph->add_attacker(asn_on_path);
            } else {
                received_from_asn = NOTHIJACKED_ASN;
            }
        } else {
            // Otherwise received it from previous AS
            received_from_asn = *(it-1);
        }

        // No break in path so send the announcement
        if (!broken_path) {
            ROVAnnouncement ann = ROVAnnouncement(*as_path->rbegin(),
                                                    prefix.addr,
                                                    prefix.netmask,
                                                    priority,
                                                    received_from_asn,
                                                    timestamp,
                                                    true);

            // Send the announcement to the current AS
            as_on_path->process_announcement(ann, this->random_tiebraking);
            if (this->graph->inverse_results != NULL) {
                auto set = this->graph->inverse_results->find(
                        std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
                // Remove the AS from the prefix's inverse results
                if (set != this->graph->inverse_results->end()) {
                    set->second->erase(as_on_path->asn);
                }
            }
        }
    }
}