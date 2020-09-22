/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#include <cmath>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <thread>
#include <chrono>
#include "Extrapolator.h"

int g_loop = 0;
int g_ts_tb = 0;
int g_broken_path = 0;
int g_verification = 0; 

Extrapolator::Extrapolator(bool invert_results,
                           bool store_depref,
                           bool origin_only,
                           bool mrt_only,
                           std::string a, 
                           std::string r, 
                           std::string i,
                           std::string d,
                           std::string v,
                           uint32_t iteration_size,
                           uint32_t verification_as) {
    invert = invert_results;                    // True to store the results inverted
    depref = store_depref;                      // True to store the second best ann for depref
    origin_o = origin_only;
    mrt_o = mrt_only;
    it_size = iteration_size;                   // Number of prefix to be precessed per iteration
    vf_as = verification_as;
    graph = new ASGraph;
    querier = new SQLQuerier(a, r, i, d, v);
}

Extrapolator::~Extrapolator(){
    delete graph;
    delete querier;
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
void Extrapolator::perform_propagation(bool test, size_t max_total){
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
    if (vf_as != false) {
        querier->clear_vf_from_db();
        querier->create_vf_tbl();
    }
    querier->clear_stubs_from_db();
    querier->clear_non_stubs_from_db();
    querier->clear_supernodes_from_db();
    querier->create_stubs_tbl();
    querier->create_non_stubs_tbl();
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
    for (auto p : *prefix_blocks) {
        delete p;
    }
    for (auto p : *subnet_blocks) {
        delete p;
    }
    delete prefix_blocks;
    delete subnet_blocks;
    
    std::cout << "Announcement count: " << announcement_count << std::endl;
    std::cout << "Verification count: " << g_verification << std::endl;
    std::cout << "Loop count: " << g_loop << std::endl;
    std::cout << "Timestamp Tiebreak count: " << g_ts_tb << std::endl;
    std::cout << "Broken Path count: " << g_broken_path << std::endl;
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
    
    // If the subnet count is within size constraint
    if (r[0][0].as<uint32_t>() < it_size) {
        // Add to subnet block vector
        if (r[0][0].as<uint32_t>() > 0) {
            Prefix<Integer>* p_copy = new Prefix<Integer>(p->addr, p->netmask);
            bloc_vector->push_back(p_copy);
        }
    } else {
        // Store the prefix if there are announcements for it specifically
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

/** Stores the verification AS's announcements for later comparison.
 *
 */
void Extrapolator::store_vf_ann(std::string prefix, 
                                uint32_t origin,
                                std::string path) {
    querier->insert_vf_ann_to_db(vf_as, prefix, origin, path);
}

/** Parse array-like strings from db to get AS_PATH in a vector.
 *
 * libpqxx doesn't currently support returning arrays, so we need to do this. 
 * path_as_string is passed in as a copy on purpose, since otherwise it would
 * be destroyed.
 *
 * @param path_as_string the as_path as a string returned by libpqxx
 * @return as_path The AS path as vector of integers
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
    while ((pos = path_as_string.find(delimiter)) != std::string::npos) {
        token = path_as_string.substr(0,pos);
        try {
            as_path->push_back(std::stoul(token));
        } catch(...) {
            std::cerr << "Parse path error, token was: " << token << std::endl;
        }
        path_as_string.erase(0,pos + delimiter.length());
    }
    // Handle last ASN after loop
    try {
        as_path->push_back(std::stoul(path_as_string));
    } catch(...) {
        std::cerr << "Parse path error, token was: " << path_as_string << std::endl;
    }
    return as_path;
}

/** Check for loops in the path and drop announcement if they exist
 */
bool Extrapolator::find_loop(std::vector<uint32_t>* as_path) {
    uint32_t prev = 0;
    bool loop = false;
    int sz = as_path->size();
    for (int i = 0; i < (sz-1) && !loop; i++) {
        prev = as_path->at(i);
        for (int j = i+1; j < sz && !loop; j++) {
            loop = as_path->at(i) == as_path->at(j) && as_path->at(j) != prev;
            prev = as_path->at(j);
        }
    }
    return loop;
}

/** Process a set of prefix or subnet blocks in iterations.
 */
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
                
                // Disclude the verification AS
                if ((*as_path)[0] == vf_as) {
                    g_verification += 1; 
                    store_vf_ann(cur_prefix.to_cidr(), origin, path_as_string);
                } else {
                    // Seed announcements along AS path
                    if (origin_o == false) {
                        give_ann_to_as_path(as_path, cur_prefix, timestamp);
                    } else {
                        give_origin_to_as_path(as_path, cur_prefix, timestamp);
                    }
                }
                delete as_path;
            }
            // Propagate for this subnet
            std::cout << "Propagating..." << std::endl;
            if (!mrt_o) {
                propagate_up();
                propagate_down();
            }
            save_results(iteration);
            graph->clear_announcements();
            iteration++;
            
            std::cout << prefix->to_cidr() << " completed." << std::endl;
            auto prefix_finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> q = prefix_finish - prefix_start;
        }
}

/** Fixes the incorrectly propagated full paths of MRT announcements.
 *
 * @param cur_as Pointer to the AS that sent old announcements.
 * @param as_path New AS path with the current AS appended.
 * @param ann_to_check_for Announcement with prefix/origin.
 */
void Extrapolator::fix_path(AS* cur_as, std::vector<uint32_t> const& as_path, Announcement const& ann_to_check_for) {
    // For each relation at the current AS, fix the path recursively
    
    // For providers
    for (uint32_t provider_asn : *cur_as->providers) {
        // For each provider, fix path
        
        // Get the neighbor
        auto neighbor = graph->ases->find(provider_asn);

        // Ensure the neighbor exists
        if (neighbor != graph->ases->end()) {
            // Get the neighbors announcement
            auto neighbor_ann = neighbor->second->all_anns->find(ann_to_check_for.prefix);
            
            // If neighbors ann exists
            if (neighbor_ann != neighbor->second->all_anns->end()) { 
                // If neighbors announcement came from cur_as
                if (neighbor_ann->second.received_from_asn == cur_as->asn) {
                    std::vector<uint32_t> new_path = as_path;
                    new_path.push_back(provider_asn);
                    neighbor_ann->second.as_path = new_path;
                    // Recursively fix path at neighbor
                    fix_path(neighbor->second, new_path, ann_to_check_for);
                }
            }
        } else {
            std::cout << "Provider doesn't exist!?" << std::endl;
        }
    }
    
    // For customers
    for (uint32_t customer_asn : *cur_as->customers) {
        // For each provider, fix path
        
        // Get the neighbor
        auto neighbor = graph->ases->find(customer_asn);
        
        // If neighbor exists
        if (neighbor != graph->ases->end()) {
            // Get the neighbors announcement
            auto neighbor_ann = neighbor->second->all_anns->find(ann_to_check_for.prefix);
            
            // If neighbors ann exists
            if (neighbor_ann != neighbor->second->all_anns->end()) { 
                // If neighbors announcement came from cur_as
                if (neighbor_ann->second.received_from_asn == cur_as->asn) {
                    std::vector<uint32_t> new_path = as_path;
                    new_path.push_back(customer_asn);
                    neighbor_ann->second.as_path = new_path;
                    // Recursively fix path at neighbor
                    fix_path(neighbor->second, new_path, ann_to_check_for);
                }
            }
        } else {
            std::cout << "Customer doesn't exist!?" << std::endl;
        }
    }

    // For peers
    for (uint32_t peer_asn : *cur_as->peers) {
        // For each provider, fix path
        
        // Get the neighbor
        auto neighbor = graph->ases->find(peer_asn);

        // If neighbor exists
        if (neighbor != graph->ases->end()) {
            // Get the neighbors announcement
            auto neighbor_ann = neighbor->second->all_anns->find(ann_to_check_for.prefix);
            
            // If neighbors ann exists
            if (neighbor_ann != neighbor->second->all_anns->end()) { 
                // If neighbors announcement came from cur_as
                if (neighbor_ann->second.received_from_asn == cur_as->asn) {
                    std::vector<uint32_t> new_path = as_path;
                    new_path.push_back(peer_asn);
                    neighbor_ann->second.as_path = new_path;
                    // Recursively fix path at neighbor
                    fix_path(neighbor->second, new_path, ann_to_check_for);
                }
            }
        } else {
            std::cout << "Peer doesn't exist!?" << std::endl;
        }
    }
}

/** Seed just the origin AS to the ASGraph.
 *
 * The from_monitor attribute is set to true on these announcements so they are
 * not replaced later during propagation. 
 *
 * @param as_path Vector of ASNs for this announcement.
 * @param prefix The prefix this announcement is for.
 */
void Extrapolator::give_origin_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp) {
    // Handle empty as_path
    if (as_path->empty()) { 
        return;
    }
    
    uint32_t origin_asn = as_path->back();
    
    // Announcement at origin for checking along the path
    Announcement ann_to_check_for(origin_asn,
                                  prefix.addr,
                                  prefix.netmask,
                                  0,
                                  timestamp); 
    
    // Full path pointer
    std::vector<uint32_t> cur_path;

    // If ASN not in graph, return
    if (graph->ases->find(origin_asn) == graph->ases->end()) {
        return;
    }

    // Translate ASN to it's supernode
    uint32_t origin_p_asn = graph->translate_asn(origin_asn);
    
    // Add origin to store the full path
    cur_path.push_back(origin_p_asn);

    // Get the origin AS
    AS *origin = graph->ases->find(origin_p_asn)->second;
    
    // Check if already received this prefix
    if (origin->already_received(ann_to_check_for)) {
        // Find the announcement for current prefix
        auto search = origin->all_anns->find(ann_to_check_for.prefix);
        
        // If the current timestamp is newer (worse)
        if (ann_to_check_for.tstamp > search->second.tstamp) {
            // Skip it
            return;
        } else if (ann_to_check_for.tstamp == search->second.tstamp) {
            // For origin only, skip it
            return;
        } else {
            // Delete newer (worse) MRT announcement, proceed with seeding
            origin->delete_ann(ann_to_check_for);
        }
    }
    
    // This is how priority is calculated
    double priority = 400;
    uint32_t received_from_asn = origin_p_asn;
    // Send the announcement
    Announcement ann = Announcement(origin_asn,
                                    prefix.addr,
                                    prefix.netmask,
                                    received_from_asn,
                                    0,
                                    priority,
                                    cur_path,
                                    timestamp,
                                    true);
    // Send the announcement to the current AS
    origin->process_announcement(ann);
    if (graph->inverse_results != NULL) {
        auto set = graph->inverse_results->find(
                std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
        // Remove the AS from the prefix's inverse results
        if (set != graph->inverse_results->end()) {
            set->second->erase(origin->asn);
        }
    }
}

/** Seed announcement on all ASes on as_path. 
 *
 * The from_monitor attribute is set to true on these announcements so they are
 * not replaced later during propagation. 
 *
 * @param as_path Vector of ASNs for this announcement.
 * @param prefix The prefix this announcement is for.
 */
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
    // Full path pointer
    std::vector<uint32_t> cur_path;
    
    // Iterate through path starting at the origin
    for (auto it = as_path->rbegin(); it != as_path->rend(); ++it) {
        // Increments path length, including prepending
        i++;
        
        // Append ASN to current full path
        cur_path.push_back(*it);
        
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
                // Random tie breaker for equal timestamp
                bool value = as_on_path->get_random();
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
                    fix_path(as_on_path, cur_path, ann_to_check_for);
                }
            } else {
                // Delete worse MRT announcement, proceed with seeding
                as_on_path->delete_ann(ann_to_check_for);
                fix_path(as_on_path, cur_path, ann_to_check_for);
            }
        }
        
        // If ASes in the path aren't neighbors (data is out of sync)
        bool broken_path = false;

        // It is 300 by default
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
                                            received_from_asn,
                                            0,
                                            priority,
                                            cur_path,
                                            timestamp,
                                            true);
            // Send the announcement to the current AS
            as_on_path->process_announcement(ann);
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
            // TODO Skip rest of path?
            //std::cerr << "Broken path for " << *(it - 1) << ", " << *it << std::endl;
            g_broken_path++;
        }
    }
}

/** Propagate announcements from customers to peers and providers ASes.
 */
void Extrapolator::propagate_up() {
    size_t levels = graph->ases_by_rank->size();
    // Propagate to providers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements();
            bool is_mh = search->second->multihome;
            bool is_empty = search->second->all_anns->empty();
            if (!is_mh && !is_empty) {
                send_all_announcements(asn, true, false, false);
            }
        }
    }
    // Propagate to peers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements();
            bool is_mh = search->second->multihome;
            bool is_empty = search->second->all_anns->empty();
            if (!is_mh && !is_empty) {
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
            auto search = graph->ases->find(asn);
            search->second->process_announcements();
            bool is_mh = search->second->multihome;
            bool is_empty = search->second->all_anns->empty();
            if (!is_mh && !is_empty) {
                send_all_announcements(asn, false, false, true);
            }
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
            
            // Full path generation
            auto cur_path = ann.second.as_path;

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
            
            // Set inference length
            uint32_t cur_len = ann.second.inference_l + 1;
            
            // Push announcement with new priority to ann vector
            anns_to_providers.push_back(Announcement(ann.second.origin,
                                                     ann.second.prefix.addr,
                                                     ann.second.prefix.netmask,
                                                     asn,
                                                     cur_len,
                                                     priority,
                                                     cur_path,
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
            
            // Full path generation
            auto cur_path = ann.second.as_path;

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
            // Set inference length
            uint32_t cur_len = ann.second.inference_l + 1;
            uint32_t priority = 100 + path_len_weight;
            
            anns_to_peers.push_back(Announcement(ann.second.origin,
                                                 ann.second.prefix.addr,
                                                 ann.second.prefix.netmask,
                                                 asn,
                                                 cur_len,
                                                 priority,
                                                 cur_path,
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
            
            // Full path generation
            auto cur_path = ann.second.as_path;
            
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
            // Set inference length
            uint32_t cur_len = ann.second.inference_l + 1;
            uint32_t priority = path_len_weight;

            anns_to_customers.push_back(Announcement(ann.second.origin,
                                                     ann.second.prefix.addr,
                                                     ann.second.prefix.netmask,
                                                     asn,
                                                     cur_len,
                                                     priority,
                                                     cur_path,
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
        std::cout << "Saving Inverse Results From Iteration: " << iteration << std::endl;
        for (auto po : *graph->inverse_results){
            for (uint32_t asn : *po.second) {
                outfile << asn << ','
                        << po.first.first.to_cidr() << ','
                        << po.first.second << '\n';
            }
        }
        outfile.close();
        querier->copy_inverse_results_to_db(file_name);
    // Handle verification results
    } else if (vf_as) {
        auto ctrl_as = *graph->ases->find(vf_as);
        ctrl_as.second->stream_announcements(outfile);
        outfile.close();
        querier->copy_results_to_db(file_name);
    // Handle standard results
    } else {
        std::cout << "Saving Results From Iteration: " << iteration << std::endl;
        for (auto &as : *graph->ases){
            as.second->stream_announcements(outfile);
        }
        outfile.close();
        querier->copy_results_to_db(file_name);
    }
    // Cleanup
    std::remove(file_name.c_str());
    
    // Handle depref results
    if (depref) {
        std::string depref_name = "/dev/shm/bgp/depref" + std::to_string(iteration) + ".csv";
        outfile.open(depref_name);
        std::cout << "Saving Depref From Iteration: " << iteration << std::endl;
        for (auto &as : *graph->ases) {
            as.second->stream_depref(outfile);
        }
        outfile.close();
        querier->copy_depref_to_db(depref_name);
        std::remove(depref_name.c_str());
    }
}
