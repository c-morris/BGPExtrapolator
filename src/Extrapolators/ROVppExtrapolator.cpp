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

#include "Extrapolators/ROVppExtrapolator.h"
#include "Graphs/ROVppASGraph.h"
#include "TableNames.h"
#include "ASes/ROVppAS.h"

ROVppExtrapolator::ROVppExtrapolator(std::vector<std::string> policy_tables,
                                        std::string announcement_table,
                                        std::string results_table,
                                        std::string simulation_table)
    : BaseExtrapolator(false, false, false) {
        
    graph = new ROVppASGraph();
    // fix rovpp extrapolation results table name, the default arg doesn't work right here
    results_table = results_table == RESULTS_TABLE ? ROVPP_RESULTS_TABLE : results_table;
    querier = new ROVppSQLQuerier(policy_tables, announcement_table, results_table, simulation_table);
}

ROVppExtrapolator::ROVppExtrapolator() : ROVppExtrapolator(std::vector<std::string>(), ROVPP_ANNOUNCEMENTS_TABLE, ROVPP_RESULTS_TABLE, ROVPP_SIMULATION_TABLE) { }

ROVppExtrapolator::~ROVppExtrapolator() { }

/** Performs propagation up and down twice. First once with the Victim prefix pairs,
 * then a second time once with the Attacker prefix pairs.
 *
 * Peforms propagation of the victim and attacker prefix pairs one at a time.
 * First victims, and then attackers. The function doesn't use subnet blocks to 
 * iterate over. Instead it does the victims table all at once, then the attackers
 * table all at once.
 *
 * If iteration block sizes need to be considered, then we need to override and use the
 * perform_propagation(bool, size_t) method instead. 
 *
 * @param propagate_twice: Don't worry about it, it's ignored
 */
void ROVppExtrapolator::perform_propagation(bool propagate_twice=true) {
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
    querier->clear_results_from_db();
    querier->create_results_tbl();
    querier->clear_supernodes_from_db();
    querier->create_supernodes_tbl();
    querier->create_rovpp_blacklist_tbl();
    
    // Generate the graph and populate the stubs & supernode tables
    graph->create_graph_from_db(querier);
    
    // Main differences start here
    std::cout << "Beginning propagation..." << std::endl;
    
    // Seed MRT announcements and propagate    
    // Iterate over Victim table (first), then Attacker table (second)
    int iter = 0;
    for (const string table_name: {querier->simulation_table}) {
        // Get the prefix-origin pairs from the database
        pqxx::result prefix_origin_pairs = querier->select_all_pairs_from(table_name);
        // Seed each of the prefix-origin pairs
        for (pqxx::result::const_iterator c = prefix_origin_pairs.begin(); c!=prefix_origin_pairs.end(); ++c) {
            // Extract Arguments needed for give_ann_to_as_path
            std::vector<uint32_t>* parsed_path = parse_path(c["as_path"].as<string>());
            Prefix<> the_prefix = Prefix<>(c["prefix_host"].as<string>(), c["prefix_netmask"].as<string>());
            int64_t timestamp = 1;  // Bogus value just to satisfy function arguments (not actually being used)
            bool is_hijack = false; //table_name == querier->attack_table;
            if (is_hijack) {
                // Add origin to attackers
                graph->attackers->insert(parsed_path->at(0));
            }
            
            // Seed the announcement
            give_ann_to_as_path(parsed_path, the_prefix, timestamp, is_hijack);
    
            // Clean up
            delete parsed_path;
        }
    }
    
    // This will propogate up and down until the graph no longer changes
    // Changes are tripped when the graph_changed variable is triggered
    int count = 0;
    do {
        ROVppAS::graph_changed = false;
        propagate_up();
        propagate_down();
        count++;
        if (count >= 100) {
            std::cout << "Exceeded max propagation cycles" << std::endl;
        }
    } while (ROVppAS::graph_changed && count < 100);
    std::cout << "Times propagated: " << count << std::endl;
    
    // std::ofstream gvpythonfile;
    // gvpythonfile.open("asgraph.py");
    // std::vector<uint32_t> to_graph = {  };
    // rovpp_graph->to_graphviz(gvpythonfile, to_graph);
    // gvpythonfile.close();
    save_results(iter);
    std::cout << "completed: ";
}

/** Seed announcement to all ASes on as_path.
 *
 * The from_monitor attribute is set to true on these announcements so they are
 * not replaced later during propagation. The ROVpp version overrides the origin
 * ASN variable at the origin AS with a flagged value for analysis.
 *
 * @param as_path Vector of ASNs for this announcement.
 * @param prefix The prefix this announcement is for.
 */
void ROVppExtrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, 
                                            Prefix<> prefix, 
                                            int64_t timestamp, 
                                            bool hijack) {
    // Handle empty as_path
    if (as_path->empty()) { 
        return;
    }
    
    uint32_t i = 0;
    uint32_t origin_asn = as_path->back();
    
    // Announcement at origin for checking along the path
    ROVppAnnouncement ann_to_check_for(origin_asn,
                                  prefix.addr,
                                  prefix.netmask,
                                  0,
                                  timestamp); 
    
    // Full path vector
    // TODO only handles seeding announcements at origin
    std::vector<uint32_t> cur_path;
    cur_path.push_back(origin_asn);
  
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
        ROVppAS *as_on_path = graph->ases->find(asn_on_path)->second;
        // Check if already received this prefix
        if (as_on_path->already_received(ann_to_check_for)) {
            // Find the already received announcement and delete it
            as_on_path->delete_ann(ann_to_check_for);
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
            ROVppAnnouncement ann = ROVppAnnouncement(*as_path->rbegin(),
                                            prefix.addr,
                                            prefix.netmask,
                                            priority,
                                            received_from_asn,
                                            timestamp,
                                            0, // policy defaults to BGP
                                            cur_path,
                                            true);
            // Send the announcement to the current AS
            std::cout << "Process Announcement" << std::endl;
            as_on_path->BaseAS::process_announcement(ann, false);
            if (graph->inverse_results != NULL) {
                auto set = graph->inverse_results->find(
                        std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
                // Remove the AS from the prefix's inverse results
                if (set != graph->inverse_results->end()) {
                    set->second->erase(as_on_path->asn);
                }
            }
        } else {
            // Report the broken path if desired
        }
    }
}

/** Withdraw given announcement at given neighbor.
 *
 * @param asn The AS issuing the withdrawal
 * @param ann The announcement to withdraw
 * @param neighbor The AS applying the withdraw
 */
void ROVppExtrapolator::process_withdrawal(uint32_t asn, ROVppAnnouncement ann, ROVppAS *neighbor) {
    // Get the neighbors announcement
    auto neighbor_ann = neighbor->loc_rib->find(ann.prefix);
    
    // If neighbors announcement came from previous AS (relevant withdrawal)
    if (neighbor_ann != neighbor->loc_rib->end() && 
        neighbor_ann->second.received_from_asn == asn) {
        // Add withdrawal to this neighbor
        neighbor->withdraw(ann);
        // Apply withdrawal by deleting ann
        neighbor->loc_rib->erase(neighbor_ann);
        // Recursively process at this neighbor
        process_withdrawals(neighbor);
    }
}

/** Handles processing all withdrawals at a particular AS. 
 *
 * @param as The AS that is sending out it's withdrawals
 */
void ROVppExtrapolator::process_withdrawals(ROVppAS *as) {
    std::vector<std::set<uint32_t>*> neighbor_set;
    neighbor_set.push_back(as->providers);
    neighbor_set.push_back(as->peers);
    neighbor_set.push_back(as->customers);

    // For each withdrawal
    for (auto withdrawal: *as->withdrawals) { 
        // For the current set
        for (auto cur_neighbors: neighbor_set) { 
            // For the current neighbor
            for (uint32_t neighbor_asn : *cur_neighbors) {
                // Get the neighbor

                // AS *neighbor = graph->ases->find(neighbor_asn)->second;
                // ROVppAS *r_neighbor = dynamic_cast<ROVppAS*>(neighbor);

                ROVppAS *r_neighbor = graph->ases->find(neighbor_asn)->second;

                // Recursively process withdrawal at neighbor
                process_withdrawal(as->asn, withdrawal, r_neighbor);
           }
        }
    }
}

/** Propagate announcements from customers to peers and providers ASes.
 */
void ROVppExtrapolator::propagate_up() {
    size_t levels = graph->ases_by_rank->size();
    // Propagate to providers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(false);
            //ROVppAS *rovpp_as = dynamic_cast<ROVppAS*>(search->second);
            //process_withdrawals(rovpp_as);
            send_all_announcements(asn, true, false, false);
        }
    }
    // Propagate to peers
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(false);
            //ROVppAS *rovpp_as = dynamic_cast<ROVppAS*>(search->second);
            //process_withdrawals(rovpp_as);
            send_all_announcements(asn, false, true, false);
        }
    }
}

/** Send "best" announces from providers to customer ASes. 
 */
void ROVppExtrapolator::propagate_down() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = levels-1; level-- > 0;) {
        for (uint32_t asn : *graph->ases_by_rank->at(level)) {
            auto search = graph->ases->find(asn);
            search->second->process_announcements(false);
            //ROVppAS *rovpp_as = dynamic_cast<ROVppAS*>(search->second);
            //process_withdrawals(rovpp_as);
            send_all_announcements(asn, false, false, true);
        }
    }
}

/** Given an announcement and index, returns priority.
 */
uint32_t ROVppExtrapolator::get_priority(ROVppAnnouncement const& ann, uint32_t i) {
    // Compute portion of priority from path length
    uint32_t path_len_weight = ann.priority % 100;
    if (path_len_weight == 0) {
        // For MRT ann at origin: old_priority = 400
        path_len_weight = 99;
    } else {
        // Sub 1 for the current hop
        path_len_weight -= 1;
    }
    return path_len_weight + (i*100);
}

/** Checks whether an announcement should be filtered out. 
 */
bool ROVppExtrapolator::is_filtered(ROVppAS *rovpp_as, ROVppAnnouncement const& ann) {
    bool filter_ann = false;
    for (auto blackhole_ann : *rovpp_as->blackholes) {
        if (blackhole_ann.prefix == ann.prefix &&
            blackhole_ann.origin == ann.origin) {
            filter_ann = true;
        }
    }
    for (auto ann_pair : *rovpp_as->preventive_anns) {
        if (ann_pair.first.prefix == ann.prefix &&
            ann_pair.first.origin == ann.origin) {
            filter_ann = true;
        }
    }
    return filter_ann;
}

/** Send all announcements kept by an AS to its neighbors. 
 *
 * This approximates the Adj-RIBs-out. ROVpp version simply replaces Announcement 
 * objects with ROVppAnnouncements.
 *
 * @param asn AS that is sending out announces
 * @param to_providers Send to providers
 * @param to_peers Send to peers
 * @param to_customers Send to customers
 */
void ROVppExtrapolator::send_all_announcements(uint32_t asn, 
                                               bool to_providers, 
                                               bool to_peers, 
                                               bool to_customers) {
    std::vector<ROVppAnnouncement> anns_to_providers;
    std::vector<ROVppAnnouncement> anns_to_peers;
    std::vector<ROVppAnnouncement> anns_to_customers;
    std::vector<std::vector<ROVppAnnouncement>*> outgoing;
    outgoing.push_back(&anns_to_customers);
    outgoing.push_back(&anns_to_peers);
    outgoing.push_back(&anns_to_providers);
    // Index for vector selection
    uint32_t i = 0;
    if (to_peers)
        i = 1;
    if (to_providers)
        i = 2;

    // Get the AS that is sending it's announcements
    // auto *source_as = graph->ases->find(asn)->second; 
    // ROVppAS *rovpp_as = dynamic_cast<ROVppAS*>(source_as);

    ROVppAS *source_as = graph->ases->find(asn)->second;
    
    // For each withdrawal
    for (auto it = source_as->withdrawals->begin(); it != source_as->withdrawals->end();) {
        // TODO This shouldn't ever happen
        if (!it->withdraw) {
            it = source_as->withdrawals->erase(it);
        // Add each withdrawal to be sent
        } else {
            // Prepare withdrawals found in withdrawals
            // Set the priority of the announcement at destination 
            uint32_t old_priority = it->priority;
            uint32_t path_len_weight = old_priority % 100;
            if (path_len_weight == 0) {
                // For MRT ann at origin: old_priority = 400
                path_len_weight = 99;
            } else {
                // Sub 1 for the current hop
                path_len_weight -= 1;
            }
            // Full path generation
            auto cur_path = it->as_path;
            // Handles appending after origin
            if (cur_path.size() == 0 || cur_path.back() != asn) {
                cur_path.push_back(asn);
            }
            // Copy announcement
            ROVppAnnouncement copy = *it;
            copy.received_from_asn = asn;
            copy.from_monitor = false;
            copy.as_path = cur_path;

            // Do not propagate any announcements from peers/providers
            // Set the priority of the announcement at destination 
            // Priority is reduced by 1 per path length
            // Ignore announcements not from a customer
            if (it->priority >= 200) {
                // Handle to providers
                // Base priority is 200 for customer to provider
                uint32_t prov_priority = 200 + path_len_weight;
                copy.priority = prov_priority;
                anns_to_providers.push_back(copy);
                
                // Handle to peers
                // Base priority is 100 for peers to peers
                uint32_t peer_priority = 100 + path_len_weight;
                copy.priority = peer_priority;
                anns_to_peers.push_back(copy);
            }
            // Handle to customers
            // Base priority is 0 for provider to customers
            uint32_t cust_priority = path_len_weight;
            copy.priority = cust_priority;
            anns_to_customers.push_back(copy);
            
            // Remove withdrawal notice
            it = source_as->withdrawals->erase(it);
        }
    }

    // Stores whether current AS needs to filter (0.2bis and 0.3)
    bool filtered = (source_as != NULL &&
                     source_as->policy_vector.size() > 0 && 
                     (source_as->policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP ||
                      source_as->policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBIS));
    
    // Process all other ann in loc_rib
    for (auto &ann : *source_as->loc_rib) {
        // ROV++ 0.1 do not forward blackhole announcements
        if (source_as != NULL && 
            ann.second.origin == 64512 && 
            source_as->policy_vector.size() > 0 &&
            source_as->policy_vector.at(0) == ROVPPAS_TYPE_ROVPP) {
            continue;
        }

        // Full path generation
        auto cur_path = ann.second.as_path;
        // Handles appending after origin
        if (cur_path.size() == 0 || cur_path.back() != asn) {
            cur_path.push_back(asn);
        }

        // Copy announcement
        ROVppAnnouncement copy = ann.second;
        copy.received_from_asn = asn;
        copy.from_monitor = false;
        copy.as_path = cur_path;

        // Do not propagate any announcements from peers/providers
        if (to_providers && ann.second.priority >= 200) {
            // Set the priority of the announcement at destination 
            // Base priority is 200 for customer to provider
            copy.priority = get_priority(ann.second, i);
            // If AS adopts 0.2bis or 0.3
            if (filtered) {
                if (!is_filtered(source_as, ann.second)) {
                    anns_to_providers.push_back(copy);
                }
            } else {
                anns_to_providers.push_back(copy);
            }
        }
        // Do not propagate any announcements from peers/providers
        if (to_peers && ann.second.priority >= 200) {
            // Base priority is 100 for peers to peers
            copy.priority = get_priority(ann.second, i);
            // If AS adopts 0.2bis or 0.3
            if (filtered) {
                if (!is_filtered(source_as, ann.second)) {
                    anns_to_peers.push_back(copy);
                }
            } else {
                anns_to_peers.push_back(copy);
            }
        }
        // Propagate all announcements to customers
        if (to_customers) {
            // Base priority is 0 for provider to customers
            copy.priority = get_priority(ann.second, i);
            anns_to_customers.push_back(copy);
        }
    }
    
    /**
    // Trim provider and peer vectors of preventive and blackhole anns for 0.3 and 0.2bis
    if (rovpp_as != NULL &&
        rovpp_as->policy_vector.size() > 0 &&
        (rovpp_as->policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP ||
        rovpp_as->policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBIS)) {
        for (auto ann_pair : *rovpp_as->preventive_anns) {
            for (auto it = anns_to_providers.begin(); it != anns_to_providers.end();) {
                if (ann_pair.first.prefix == it->prefix &&
                    ann_pair.first.origin == it->origin) {
                    it = anns_to_providers.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto it = anns_to_peers.begin(); it != anns_to_peers.end();) {
                if (ann_pair.first.prefix == it->prefix &&
                    ann_pair.first.origin == it->origin) {
                    it = anns_to_peers.erase(it);
                } else {
                    ++it;
                }
            }
        }
        for (auto blackhole_ann : *rovpp_as->blackholes) {
            for (auto it = anns_to_providers.begin(); it != anns_to_providers.end();) {
                if (blackhole_ann.prefix == it->prefix &&
                    blackhole_ann.origin == it->origin) {
                    it = anns_to_providers.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto it = anns_to_peers.begin(); it != anns_to_peers.end();) {
                if (blackhole_ann.prefix == it->prefix &&
                    blackhole_ann.origin == it->origin) {
                    it = anns_to_peers.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
    }
    */

    // Send the vectors of assembled announcements
    for (uint32_t provider_asn : *source_as->providers) {
        // For each provider, give the vector of announcements
        auto *recving_as = graph->ases->find(provider_asn)->second;
        // NOTE SENT TO ASN IS NO LONGER SET
        recving_as->receive_announcements(anns_to_providers);
    }
    for (uint32_t peer_asn : *source_as->peers) {
        // For each provider, give the vector of announcements
        auto *recving_as = graph->ases->find(peer_asn)->second;
        recving_as->receive_announcements(anns_to_peers);
    }
    if (source_as != NULL && source_as->policy_vector.size() > 0 &&
        source_as->policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP) {
        // For ROVPPBP v3.1 we want to keep track of which Customers
        // are receiving the preventive announcements, and be able to 
        // withdraw them if they send us the prefix. Moreover, not send it
        // to them at all if they send us the prefix to begin with. The simplest
        // way of acheiving these two goals is to send them the preventive ann
        // to the customer anyway and include a withdraw ann to immediately remove it.
        std::vector<uint32_t> customer_asn_sent_prefix;
        for (ROVppAnnouncement curr_ann : *source_as->passed_rov) {
            if (curr_ann.prefix.netmask == 0xFFFF0000) {
                customer_asn_sent_prefix.push_back(curr_ann.received_from_asn);
            }
        }
        // Create withdraws for preventive ann in anns_to_customers
        // if there are any there
        std::vector<ROVppAnnouncement> preventive_ann_withdraws;
        for (auto ann_pair : *source_as->preventive_anns) {
            for (ROVppAnnouncement to_cust_ann : anns_to_customers) {
                if (ann_pair.first.prefix == to_cust_ann.prefix &&
                    ann_pair.first.origin == to_cust_ann.origin) {
                    // Create Withdraw Ann
                    ROVppAnnouncement copy = to_cust_ann;
                    copy.withdraw = true;
                    // Add it to the list of withdraws to send for preventive anns
                    preventive_ann_withdraws.push_back(copy);
                    break;
                }
            }
        }
        // Send Announcements to Customers, but inject preventive announcemnt 
        // withdraws to the anns_to_customers if the AS shared the prefix
        // with this AS (source_as || rovpp_as)
        for (uint32_t customer_asn : *source_as->customers) {
            // For each customer, give the vector of announcements
            auto *recving_as = graph->ases->find(customer_asn)->second;
            // Copy vector of anns to send to customer
            std::vector<ROVppAnnouncement> copy_anns_to_customers = anns_to_customers;
            // Check if the ASN of this AS in included in the list of customer_asn_sent_prefix
            // If that is so, then we need to include preventive_ann_withdraws for this customer
            if ( std::find(customer_asn_sent_prefix.begin(), customer_asn_sent_prefix.end(), recving_as->asn) != customer_asn_sent_prefix.end() ) {
                for (ROVppAnnouncement prev_ann_withdraw : preventive_ann_withdraws) {
                    copy_anns_to_customers.push_back(prev_ann_withdraw);
                }
            }
            recving_as->receive_announcements(copy_anns_to_customers);
        }
    } else {
        for (uint32_t customer_asn : *source_as->customers) {
            // For each customer, give the vector of announcements
            auto *recving_as = graph->ases->find(customer_asn)->second;
            recving_as->receive_announcements(anns_to_customers);
        }
    }

    // TODO Remove this?
    // Clear withdrawals except for withdrawals
    for (auto it = source_as->withdrawals->begin(); it != source_as->withdrawals->end();) {
        if (!it->withdraw) {
            it = source_as->withdrawals->erase(it);
        } else {
            ++it;
        }
    }
}

/** Check for a loop in the AS path using traceback.
 *
 * @param  p Prefix to check for
 * @param  a The ASN that, if seen, will mean we have a loop
 * @param  cur_as The current AS we are at in the traceback
 * @param  d The current depth of the search
 * @return true if a loop is detected, else false
 */
bool ROVppExtrapolator::loop_check(Prefix<> p, const ROVppAS& cur_as, uint32_t a, int d) {
    if (d > 100) { std::cerr << "Maximum depth exceeded during traceback.\n"; return true; }
    auto ann_pair = cur_as.loc_rib->find(p);
    const Announcement &ann = ann_pair->second;
    // i wonder if a cabinet holding a subwoofer counts as a bass case
    // Ba dum tss, nice
    if (ann.received_from_asn == a) { return true; }
    if (ann.received_from_asn == 64512 ||
        ann.received_from_asn == 64513 ||
        ann.received_from_asn == 64514) {
        return false;
    }
    if (ann_pair == cur_as.loc_rib->end()) { 
        return false; 
    }
    auto next_as_pair = graph->ases->find(ann.received_from_asn);
    if (next_as_pair == graph->ases->end()) { std::cerr << "Traced back announcement to nonexistent AS.\n"; return true; }
    const ROVppAS& next_as = *next_as_pair->second;
    return loop_check(p, next_as, a, d+1);
}

/** Saves the results of the extrapolation. ROVpp version uses the ROVppQuerier.
 */
void ROVppExtrapolator::save_results(int iteration) {
    // Setup output file stream
    std::ofstream outfile;
    std::ofstream blackhole_outfile;
    std::string file_name = "/dev/shm/bgp/" + std::to_string(iteration) + ".csv";
    std::string blackhole_file_name = "/dev/shm/bgp/blackholes_table_" + std::to_string(iteration) + ".csv";
    outfile.open(file_name);
    blackhole_outfile.open(blackhole_file_name);
    
    // Iterate over all nodes in graph
    std::cout << "Saving Results From Iteration: " << iteration << std::endl;
    for (auto &as : *graph->ases){
        as.second->stream_announcements(outfile);
        // Check if it's a ROVpp node
        // if (ROVppAS* rovpp_as = dynamic_cast<ROVppAS*>(as.second)) {
        if (ROVppAS* rovpp_as = as.second) {
          // It is, so now output the blacklist
          rovpp_as->stream_blackholes(blackhole_outfile);
        }
    }
      
    outfile.close();
    blackhole_outfile.close();
    querier->copy_results_to_db(file_name);
    querier->copy_blackhole_list_to_db(blackhole_file_name);
    std::remove(file_name.c_str());
    std::remove(blackhole_file_name.c_str());
}
