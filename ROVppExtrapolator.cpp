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

#include "ROVppExtrapolator.h"
#include "ROVppASGraph.h"
#include "TableNames.h"


ROVppExtrapolator::ROVppExtrapolator(std::string r,
                           std::string e,
                           std::string f,
                           std::string g,
                           std::string h,
                           std::string j,
                           uint32_t iteration_size)
    : Extrapolator(new ROVppASGraph(), new ROVppSQLQuerier(e, f, g, h, j), iteration_size) {
    // Replace the ASGraph and SQLQuerier
    // ROVpp specific functions should use the rovpp_graph variable
    // The graph variable maintains backwards compatibility
}

ROVppExtrapolator::~ROVppExtrapolator() {}

/** Performs propogation up and down twice. First once with the Victim prefix pairs,
 * then a second time once with the Attacker prefix pairs.
 *
 * Peforms propogation of the victim and attacker prefix pairs one at a time.
 * First victims, and then attackers. The function doesn't use subnet blocks to 
 * iterate over. Instead it does the victims table all at once, then the attackers
 * table all at once.
 *
 * If iteration block sizes need to be considered, then we need to override and use the
 * perform_propagation(bool, size_t) method instead. 
 */
void ROVppExtrapolator::perform_propagation() {
    // Main Differences:
    //   No longer need to consider prefix and subnet blocks
    //   No longer printing out ann count, loop counts, tiebreak information, broken path count
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
    
    // Main differences start here
    std::cout << "Beginning propagation..." << std::endl;
    
    // Seed MRT announcements and propagate    
    // Iterate over Victim table (first), then Attacker table (second)
    int iter = 0;
    for (const string table_name: {VICTIM_TABLE, ATTACKER_TABLE}) {
        // Get the prefix-origin pairs from the database
        pqxx::result prefix_origin_pairs = querier->select_all_pairs_from(table_name);
        // Seed each of the prefix-origin pairs
        for (pqxx::result::const_iterator c = prefix_origin_pairs.begin(); c!=prefix_origin_pairs.end(); ++c) {
            // Extract Arguments needed for give_ann_to_as_path
            std::vector<uint32_t>* parsed_path = parse_path(c["as_path"].as<string>());
            Prefix<> the_prefix = Prefix<>(c["prefix_host"].as<string>(), c["prefix_netmask"].as<string>());
            int64_t timestamp = 1;  // Bogus value just to satisfy function arguments (not actually being used)
            bool is_hijack = table_name == ATTACKER_TABLE;
            
            // Seed the announcement
            give_ann_to_as_path(parsed_path, the_prefix, timestamp, is_hijack);
    
            // Clean up
            delete parsed_path;
        }
        
        // Propogate the seeded announcements
        propagate_up();
        propagate_down();
        save_results(iter);
        iter++;
    }
    
    std::cout << "completed: ";
}

void ROVppExtrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp, bool hijack) {
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
                }
            } else {
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
        
        // ROV++ Handle origin received_from here
        // BHOLED = 64512
        // HIJACKED = 64513
        // NOTHIJACKED = 64514
        // PREVENTATIVEHIJACKED = 64515
        // PREVENTATIVENOTHIJACKED = 64516
        
        // If this AS is the origin
        if (it == as_path->rbegin()){
            // ROVpp: Set the recv_from to proper flag
            if (hijack) {
                received_from_asn = 64513;
            } else {
                received_from_asn = 64514;
            }
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
            //std::cerr << "Broken path for " << *(it - 1) << ", " << *it << std::endl;
        }
    }
}

void ROVppExtrapolator::save_results(int iteration) {
    std::ofstream outfile;
    std::string file_name = "/dev/shm/bgp/" + std::to_string(iteration) + ".csv";
    outfile.open(file_name);
    
    // Handle standard results
    std::cout << "Saving Results From Iteration: " << iteration << std::endl;
    for (auto &as : *graph->ases){
        as.second->stream_announcements(outfile);
    }
    outfile.close();
    querier->copy_results_to_db(file_name);
    
    std::remove(file_name.c_str());
 
}
