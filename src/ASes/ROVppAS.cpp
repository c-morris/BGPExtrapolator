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

#include "ASes/ROVppAS.h"

bool ROVppAS::graph_changed = false;

ROVppAS::ROVppAS(uint32_t myasn,
                 std::set<uint32_t> *rovpp_attackers,
                 std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inv,
                 std::set<uint32_t> *prov,
                 std::set<uint32_t> *peer,
                 std::set<uint32_t> *cust)
    : BaseAS(myasn, inv, prov, peer, cust)  {
    // Save reference to attackers
    attackers = rovpp_attackers;
    bad_neighbors = new std::set<uint32_t>;
    failed_rov = new std::set<ROVppAnnouncement>;
    passed_rov = new std::set<ROVppAnnouncement>;
    blackholes = new std::set<ROVppAnnouncement>;
    preventive_anns = new std::set<std::pair<ROVppAnnouncement, ROVppAnnouncement>>;

    ribs_in = new std::vector<ROVppAnnouncement>;
    loc_rib = new std::map<Prefix<>, ROVppAnnouncement>;
    withdrawals = new std::vector<ROVppAnnouncement>;
}

ROVppAS::~ROVppAS() { 
    delete bad_neighbors;
    delete failed_rov;
    delete passed_rov;
    delete blackholes;
    delete preventive_anns;
    
    delete ribs_in;
    delete loc_rib;
    delete withdrawals;
}

/** Adds a policy to the policy_vector
 *
 * This function allows you to specify the policies
 * that this AS implements. The different types of policies are listed in 
 * the header of this class. 
 * 
 * @param p The policy to add. For example, ROVPPAS_TYPE_BGP (defualt), and
 */
void ROVppAS::add_policy(uint32_t p) {
    policy_vector.push_back(p);
}

/** Checks whether or not an rovannouncement is from an attacker
 * 
 * @param  ann  rovannouncement to check if it passes ROV
 * @return bool  return false if from attacker, true otherwise
 */
bool ROVppAS::pass_rov(ROVppAnnouncement &ann) {
    if (ann.origin == UNUSED_ASN_FLAG_FOR_BLACKHOLES) { return false; }
    if (attackers != NULL) {
        return (attackers->find(ann.origin) == attackers->end());
    } else {
        return true;
    }
}

/** Add the rovannouncement to the vector of withdrawals to be processed.
 *
 * Also remove it from the ribs_in.
 */
void ROVppAS::withdraw(ROVppAnnouncement &ann) {
    ROVppAnnouncement copy = ann;
    copy.withdraw = true;
    withdrawals->push_back(copy);
    ROVppAS::graph_changed = true;  // This means we will need to do another propagation
}

/** Processes a single rovannouncement, adding it to the ASes set of announcements if appropriate.
 *
 * Approximates BGP best path selection based on rovannouncement priority.
 * Called by process_announcements and Extrapolator.give_ann_to_as_path()
 * 
 * @param ann The rovannouncement to be processed
 */ 
void ROVppAS::process_announcement(ROVppAnnouncement &ann, bool ran, bool override) {
    std::cout << "ROV version" << std::endl;

    // Check for existing rovannouncement for prefix
    auto search = loc_rib->find(ann.prefix);
    auto search_depref = depref_anns->find(ann.prefix);
    
    // No rovannouncement found for incoming rovannouncement prefix
    if (search == loc_rib->end()) {
        loc_rib->insert(std::pair<Prefix<>, ROVppAnnouncement>(ann.prefix, ann));
        // Inverse results need to be computed also with announcements from monitors
        if (inverse_results != NULL) {
            auto set = inverse_results->find(
                std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
            if (set != inverse_results->end()) {
                set->second->erase(asn);
            }
        }
    // If overrid is true, the replace whatever is the current rovannouncement regardless
    } else if (override) {
      /*// Disabled this to get some runs to see if we get data plane loops (this block may need to be deleted)
      // May 13, 2020: Reynaldo Morillo 
      // Update inverse results
      swap_inverse_result(
          std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
          std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
      // Insert depref ann
      depref_anns->insert(std::pair<Prefix<>, ROVppAnnouncement>(search->second.prefix, 
                                                            search->second));
      withdraw(search->second);
      search->second = ann;
      */
    // Tiebraker for equal priority between old and new ann (but not if they're the same ann)
    } else if (ann.priority == search->second.priority && ann != search->second) {
        // Random tiebraker
        //std::minstd_rand ran_bool(asn);
        ran = false;
        bool value = (ran ? get_random() : tiny_hash(ann.received_from_asn) < tiny_hash(search->second.received_from_asn) );
        // TODO This sets first come, first kept
        // value = false;
        if (value) {
            // Use the new rovannouncement and record it won the tiebreak
            if (search_depref == depref_anns->end()) {
                // Update inverse results
                swap_inverse_result(
                    std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                    std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                // Insert depref ann
                depref_anns->insert(std::pair<Prefix<>, ROVppAnnouncement>(search->second.prefix, 
                                                                      search->second));
                withdraw(search->second);
                search->second = ann;
                check_preventives(search->second);
            } else {
                swap_inverse_result(
                    std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                    std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                search_depref->second = search->second;
                withdraw(search->second);
                search->second = ann;
                check_preventives(search->second);
            }
        } else {
            // Use the old rovannouncement
            if (search_depref == depref_anns->end()) {
                depref_anns->insert(std::pair<Prefix<>, ROVppAnnouncement>(ann.prefix, 
                                                                      ann));
            } else {
                // Replace second best with the old priority rovannouncement
                search_depref->second = ann;
            }
        }
    // Otherwise check new announcements priority for best path selection
    } else if (ann.priority > search->second.priority) {
        if (search_depref == depref_anns->end()) {
            // Update inverse results
            swap_inverse_result(
                std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
            // Insert new second best rovannouncement
            depref_anns->insert(std::pair<Prefix<>, ROVppAnnouncement>(search->second.prefix, 
                                                                  search->second));
            // Replace the old rovannouncement with the higher priority
            withdraw(search->second);
            search->second = ann;
            check_preventives(search->second);
        } else {
            // Update inverse results
            swap_inverse_result(
                std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
            // Replace second best with the old priority rovannouncement
            search_depref->second = search->second;
            // Replace the old rovannouncement with the higher priority
            withdraw(search->second);
            search->second = ann;
            check_preventives(search->second);
        }
    // Old rovannouncement was better
    } else {
        if (search_depref == depref_anns->end()) {
            // Insert new second best annoucement
            depref_anns->insert(std::pair<Prefix<>, ROVppAnnouncement>(ann.prefix, ann));
        } else if (ann.priority > search_depref->second.priority) {
            // Replace the old depref rovannouncement with the higher priority
            search_depref->second = search->second;
        }
    }
}

/** Iterate through ribs_in and keep only the best. 
 */
void ROVppAS::process_announcements(bool ran) {
    // Filter ribs_in for loops, checking path for self
    for (auto it = ribs_in->begin(); it != ribs_in->end();) {
        bool deleted = false;
        for (uint32_t a : it->as_path) {
            if (a == asn && it->origin != asn) {
                it = ribs_in->erase(it);
                deleted = true;
                break;
            }
        }
        if (!deleted) {
            ++it;
        }
    }

    // Process all withdrawals in the ribs_in
    bool something_removed = false;
    do {
        something_removed = false;
        auto ribs_in_copy = *ribs_in;
        int i = 0;
        for (auto it = ribs_in_copy.begin(); it != ribs_in_copy.end(); ++it) {
            bool should_cancel = false;
            // For each withdrawal
            if (it->withdraw) {
                if (asn == 3301) {
                    //std::cout << "Cancelling withdraw" << *it << '\n';
                }
                // Determine if cancellation should occur
                for (int j = 0; j <= i && j < ribs_in->size(); j++) {
                    // Indicates there is a ann for the withdrawal to apply to
                    auto ann = ribs_in->at(j);
                    if (!ann.withdraw && ann == *it) {
                        should_cancel = true;
                        break;
                    }
                }
                if (should_cancel) {
                    auto search = loc_rib->find(it->prefix);
                    // Process withdrawal if it applies to loc_rib
                    if (search != loc_rib->end() && search->second == *it) {
                        withdraw(search->second);
                        // Put the best alternative rovannouncement into the loc_rib
                        ROVppAnnouncement best_alternative_ann = best_alternative_route(search->second); 
                        if (search->second != best_alternative_ann) {
                            search->second = best_alternative_ann;
                        } else {
                            loc_rib->erase(it->prefix);    
                        }
                        ROVppAS::graph_changed = true;  // This means we will need to do another propagation
                    }
                    // Process withdrawal in the ribs_in
                    for (auto it2 = ribs_in->begin(); it2 != ribs_in->end();) {
                        // Remove any real ann and withdrawal itself
                        if (*it2 == *it) {
                            it2 = ribs_in->erase(it2);
                            something_removed = true;
                            // decrement index
                            i--;
                        } else {
                            ++it2;
                        }
                    }
                }
            }
            i++;
        }
    } while (something_removed);
    
    // Apply "holes" to prefix announcements (i.e. good ann)
    // that came from neighbors that have sent as an attacker's ann
    // First, identify neighbors that have sent attacker announcemnts
    for (auto &ann : *ribs_in) {
        if (!pass_rov(ann)) {
            bad_neighbors->insert(ann.received_from_asn);
        }
    }
    
    // Process the ribs_in
    for (auto &ann : *ribs_in) {
        auto search = loc_rib->find(ann.prefix);
        // TODO Remove this?
        // Withdrawals should be processed already above
        // Process withdrawals, regardless of policy
        if (ann.withdraw) {
            if (search != loc_rib->end() && search->second == ann) {
                withdraw(search->second);
                // Put the best alternative rovannouncement into the loc_rib
                ROVppAnnouncement best_alternative_ann = best_alternative_route(search->second); 
                if (search->second != best_alternative_ann) {
                    search->second = best_alternative_ann;
                } else {
                    loc_rib->erase(ann.prefix);    
                }
                ROVppAS::graph_changed = true;  // This means we will need to do another propagation
                
            }
            continue;
        }
        // If loc_rib rovannouncement isn't a seeded rovannouncement
        if (search == loc_rib->end() || !search->second.from_monitor) {
            // Regardless of policy, if the rovannouncement originates from this AS
            // *or is a subprefix of its own prefix*
            // drop it
            if (ann.origin == asn && attackers->find(asn) == attackers->end()) { continue; }
            for (auto rib_ann : *loc_rib) {
                if (ann.prefix.contained_in_or_equal_to(rib_ann.second.prefix) &&
                    rib_ann.second.origin == asn &&
                    attackers->find(asn) == attackers->end()) {
                    ann.received_from_asn=64514;
                }
            }
            // If we have a policy adopted
            if (policy_vector.size() > 0) {
                // Basic ROV
                if (policy_vector.at(0) == ROVPPAS_TYPE_ROV) {
                    if (pass_rov(ann)) {
                        passed_rov->insert(ann);
                        process_announcement(ann, false);
                    }
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPP0) {
                    // The policy for ROVpp 0 is similar to ROVpp 1 
                    // Just doesn't creat blackholes
                    if (pass_rov(ann)) {
                        passed_rov->insert(ann);
                        process_announcement(ann, false);
                    } else {
                        failed_rov->insert(ann);
                        ROVppAnnouncement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // If no alternative
                            process_announcement(ann, false);
                        } else {
                            process_announcement(best_alternative_ann, false);
                        }
                    }
                // ROV++ V0.1
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPP) {
                    // The policy for ROVpp 0.1 is similar to ROV in the extrapolator.
                    // Only in the data plane changes
                    if (pass_rov(ann)) {
                        passed_rov->insert(ann);
                        // Add received from bad neighbor flag (i.e. alt flag repurposed)
                        if (bad_neighbors->find(ann.received_from_asn) != bad_neighbors->end()) {
                            ann.alt = ATTACKER_ON_ROUTE_FLAG;
                        }
                        process_announcement(ann, false);
                    } else {
                        failed_rov->insert(ann);
                        ROVppAnnouncement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // If no alternative
                            blackholes->insert(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann, false);
                        } else {
                            process_announcement(best_alternative_ann, false);
                        }
                    }
                // ROV++ V0.2
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPB) {
                    // For ROVpp 0.2, forward a blackhole ann if there is no alt route.
                    if (pass_rov(ann)) {
                        passed_rov->insert(ann);
                        // Add received from bad neighbor flag (i.e. alt flag repurposed)
                        if (bad_neighbors->find(ann.received_from_asn) != bad_neighbors->end()) {
                            ann.alt = ATTACKER_ON_ROUTE_FLAG;
                        }
                        process_announcement(ann, false);
                    } else {
                        failed_rov->insert(ann);
                        ROVppAnnouncement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // If no alternative
                            // Mark as blackholed and accept this rovannouncement
                            blackholes->insert(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann, false);
                        } else {
                            process_announcement(best_alternative_ann, false);
                        }
                    }
                /* // Temporarily comment out to test out new v0.2bis   
                // ROV++ V0.2bis
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBIS) {
                    // For ROVpp 0.2bis, forward a blackhole ann to customers if there is no alt route.
                    if (pass_rov(ann)) {
                        passed_rov->insert(ann);
                        process_announcement(ann, false);
                    } else {
                        failed_rov->insert(ann);
                        ROVppAnnouncement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // If no alternative
                            // Mark as blackholed and accept this rovannouncement
                            blackholes->insert(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann, false);
                        } else {
                            process_announcement(best_alternative_ann, false);
                        }
                    }
                
                 */
                // New ROV++ V0.2bis (drops hijack announcements silently like v0.3)
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBIS) {
                    // For ROVpp 0.2bis, forward a blackhole ann to customers if there is no alt route.
                    if (pass_rov(ann)) {
                        passed_rov->insert(ann);
                        // Add received from bad neighbor flag (i.e. alt flag repurposed)
                        if (bad_neighbors->find(ann.received_from_asn) != bad_neighbors->end()) {
                            ann.alt = ATTACKER_ON_ROUTE_FLAG;
                        }
                        process_announcement(ann, false);
                    } else {
                        // If it is from a customer, silently drop it
                        if (customers->find(ann.received_from_asn) != customers->end()) { continue; }
                        failed_rov->insert(ann);
                        ROVppAnnouncement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // If no alternative
                            // Mark as blackholed and accept this rovannouncement
                            blackholes->insert(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann, false);
                        } else {
                            process_announcement(best_alternative_ann, false);
                        }
                    }
                // ROV++ V0.3 (atually this is 3bis [or experiment to get rid of loops])
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP) {
                    // For ROVpp 0.3, forward a blackhole ann if there is no alt route.
                    // Also make a preventive rovannouncement if there is an alt route.
                    if (pass_rov(ann)) {
                        passed_rov->insert(ann);
                        // Add received from bad neighbor flag (i.e. alt flag repurposed)
                        if (bad_neighbors->find(ann.received_from_asn) != bad_neighbors->end()) {
                            ann.alt = ATTACKER_ON_ROUTE_FLAG;
                        }
                        process_announcement(ann);
                    } else {
                        // If it is from a customer, silently drop it
                        if (customers->find(ann.received_from_asn) != customers->end()) { continue; }
                        ROVppAnnouncement best_alternative_ann = best_alternative_route(ann); 
                        failed_rov->insert(ann);
                        if (best_alternative_route(ann) == ann) { // If no alternative
                            // Mark as blackholed and accept this rovannouncement
                            blackholes->insert(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann);
                        } else {
                            // Move entire prefix to alternative
                            process_announcement(best_alternative_ann, false, true);
                            // Make preventive rovannouncement
                            ROVppAnnouncement preventive_ann = best_alternative_ann;
                            preventive_ann.prefix = ann.prefix;
                            preventive_ann.alt = best_alternative_ann.received_from_asn;
                            if (preventive_ann.origin == asn) { preventive_ann.received_from_asn=64514; }
                            preventive_anns->insert(std::pair<ROVppAnnouncement,ROVppAnnouncement>(preventive_ann, best_alternative_ann));
                            process_announcement(preventive_ann);
                        }
                    }
                } else { // Unrecognized policy defaults to bgp
                    process_announcement(ann, false);
                }
            } else { // If there is no policy, default to bgp
                process_announcement(ann, false);
            }
        }
    }
    
    // TODO Remove this?
    // Withdrawals are deleted after processing above
    // Remove withdrawals
    for (auto it = ribs_in->begin(); it != ribs_in->end();) {
        if (it->withdraw) {
            it = ribs_in->erase(it);
        } else {
            ++it;
        }
    }
}

/** Will return the best alternative announcemnt if it exists. If it doesn't exist, it will return the 
 * rovannouncement it was given.
 * 
 * @param  ann An announcemnt you want to find an alternative for.
 * @return     The best alternative rovannouncement (i.e. an rovannouncement which came from a neighbor who hadn't shared
 *             an attacker announcemnt with us).
 */
 ROVppAnnouncement ROVppAS::best_alternative_route(ROVppAnnouncement &ann) {
     // Initialize the default answer of (No best alternative with the current given ann)
     // This variable will update with the best ann if it exists
     ROVppAnnouncement best_alternative_ann = ann;
     // Create an ultimate list of good candidate announcemnts (ribs_in)
     std::vector<ROVppAnnouncement> candidates;
     std::set<ROVppAnnouncement> baddies = *failed_rov;
     // For each possible alternative
     for (auto candidate_ann : *ribs_in) {
        if (pass_rov(candidate_ann) && !candidate_ann.withdraw && 
            candidate_ann.alt != ATTACKER_ON_ROUTE_FLAG) {
           candidates.push_back(candidate_ann);
        } else {
           baddies.insert(candidate_ann);
        }
     }
     // Find the best alternative to ann
     for (auto &candidate : candidates) {
         // Is there a valid alternative?
         if (ann.prefix.contained_in_or_equal_to(candidate.prefix)) {
             // Is the candidate safe?
             bool safe = true;
             for (auto &curr_bad_ann : baddies) {
                 if (curr_bad_ann.prefix.contained_in_or_equal_to(candidate.prefix) &&
                     curr_bad_ann.received_from_asn == candidate.received_from_asn) {
                     // Well yes, but actually no
                     safe = false;
                     break;
                 }
             }
             if (safe) {
                 // Always replace the initial bad ann if we have an alternative
                 // Else check for one with a higher priority
                 if (best_alternative_ann == ann) {
                     best_alternative_ann = candidate;
                 } else if (best_alternative_ann.priority < candidate.priority) {
                     best_alternative_ann = candidate;
                 }
             }
         }
    }
    return best_alternative_ann;
}

void ROVppAS::check_preventives(ROVppAnnouncement ann) {
    // ROV++ V0.3
    if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP) {
        // note this only works for /24...
        if (ann.prefix.netmask == 0xffffff00) {
            // this is already a preventive
            return;
        }
        ann.prefix.netmask = 0xffffff00;
        // find the preventive ann if it exists
        auto search = loc_rib->find(ann.prefix);
        if (search != loc_rib->end()) {
            ROVppAnnouncement best_alternative_ann = best_alternative_route(search->second); 
            if (ann.received_from_asn == search->second.received_from_asn) {
                // Remove and attempt to replace the preventive ann
                withdrawals->push_back(ann);
                loc_rib->erase(ann.prefix);    
                ann.withdraw = false;
                // replace
                if (best_alternative_route(ann) == ann) { // If no alternative
                    // Mark as blackholed and accept this rovannouncement
                    blackholes->insert(ann);
                    ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                    ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                    process_announcement(ann);
                } else {
                    // Make preventive rovannouncement
                    ROVppAnnouncement preventive_ann = best_alternative_ann;
                    preventive_ann.prefix = ann.prefix;
                    preventive_ann.alt = best_alternative_ann.received_from_asn;
                    if (preventive_ann.origin == asn) { preventive_ann.received_from_asn=64514; }
                    preventive_anns->insert(std::pair<ROVppAnnouncement,ROVppAnnouncement>(preventive_ann, best_alternative_ann));
                    process_announcement(preventive_ann);
                }
            }
        }
    }
}

/** Tiny galois field hash with a fixed key of 3.
 */
uint8_t ROVppAS::tiny_hash(uint32_t as_number) {
    uint8_t mask = 0xFF;
    uint8_t value = 0;
    for (int i = 0; i < sizeof(asn); i++) {
        value = (value ^ mask & (as_number>>(i * 8))) * 3;
    }
    return value;
}

/**
 * [ROVppAS::stream_blacklist description]
 * @param  os [description]
 * @return    [description]
 */
std::ostream& ROVppAS::stream_blackholes(std:: ostream &os) {
  for (ROVppAnnouncement ann : *blackholes) {
      os << asn << ",";
      ann.to_blackholes_csv(os);
  }
  return os;
}