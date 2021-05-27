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

#include "ROVppAS.h"

ROVppAS::ROVppAS(uint32_t myasn,
                 std::set<uint32_t> *rovpp_attackers,
                 std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inv,
                 std::set<uint32_t> *prov,
                 std::set<uint32_t> *peer,
                 std::set<uint32_t> *cust)
    : AS(myasn, inv, prov, peer, cust)  {
    // Save reference to attackers
    attackers = rovpp_attackers;
    bad_neighbors = new std::set<uint32_t>;
    failed_rov = new std::set<Announcement>;
    passed_rov = new std::set<Announcement>;
    blackholes = new std::set<Announcement>;
    preventive_anns = new std::set<std::pair<Announcement,Announcement>>;  
}

ROVppAS::~ROVppAS() { 
    delete bad_neighbors;
    delete failed_rov;
    delete passed_rov;
    delete blackholes;
    delete preventive_anns;
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

/** Checks if the announcement is valid by ROV AS.
 * 
 * @param  ann  announcement to check if it passes ROV
 * @return bool  return false if from attacker, true otherwise
 */
bool ROVppAS::pass_rov(Announcement &ann) {
    return ann.roa_validity == ROA_VALID || ann.roa_validity == ROA_UNKNOWN;
}


/** Checks if the announcement is valid
 * 
 * @param  ann  announcement to check if it passes ROV
 * @return bool  return false if from attacker, true otherwise
 */
bool ROVppAS::pass_rovpp(Announcement &ann) {
    return this->pass_rov(ann);
}

/** Add the announcement to the vector of withdrawals to be processed.
 *
 * Also remove it from the ribs_in.
 */
void ROVppAS::withdraw(Announcement &ann) {
    Announcement copy = ann;
    copy.withdraw = true;
    withdrawals->push_back(copy);
    AS::graph_changed = true;  // This means we will need to do another propagation
}

/** Processes a single announcement, adding it to the ASes set of announcements if appropriate.
 *
 * Approximates BGP best path selection based on announcement priority.
 * Called by process_announcements and Extrapolator.give_ann_to_as_path()
 * 
 * @param ann The announcement to be processed
 */ 
void ROVppAS::process_announcement(Announcement &ann, bool ran) {
    // Check for existing announcement for prefix
    auto search = loc_rib->find(ann.prefix);
    auto search_depref = depref_anns->find(ann.prefix);
    uint32_t relationship = ann.priority / 100;
    uint32_t policy = policy_vector.at(0); 
    
    // No announcement found for incoming announcement prefix
    if (search == loc_rib->end()) {
        loc_rib->insert(std::pair<Prefix<>, Announcement>(ann.prefix, ann));
        // Inverse results need to be computed also with announcements from monitors
        if (inverse_results != NULL) {
            auto set = inverse_results->find(
                std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
            if (set != inverse_results->end()) {
                set->second->erase(asn);
            }
        }
    } else if ((policy == ROVPPAS_TYPE_BGP || 
                policy == ROVPPAS_TYPE_ROV ||
                policy == ROVPPAS_TYPE_ROVPP_LITE || 
                policy == ROVPPAS_TYPE_ROVPPB_LITE ||
                policy == ROVPPAS_TYPE_ROVPPBP_LITE ||
                policy == ROVPPAS_TYPE_ROVPPBIS_LITE) && 
               (ann.priority == search->second.priority && ann != search->second)) {
        // Random tiebraker
        // std::minstd_rand ran_bool(asn);
        ran = false;
        bool value = (ran ? get_random() : tiny_hash(ann.received_from_asn) < tiny_hash(search->second.received_from_asn) );
        // TODO This sets first come, first kept
        // value = false;
        if (value) {
            // Use the new announcement and record it won the tiebreak
            if (search_depref == depref_anns->end()) {
                // Update inverse results
                swap_inverse_result(
                    std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                    std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                // Insert depref ann
                depref_anns->insert(std::pair<Prefix<>, Announcement>(search->second.prefix, 
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
            // Use the old announcement
            if (search_depref == depref_anns->end()) {
                depref_anns->insert(std::pair<Prefix<>, Announcement>(ann.prefix, 
                                                                      ann));
            } else {
                // Replace second best with the old priority announcement
                search_depref->second = ann;
            }
        }
    // Tiebraker for equal relationship between old and new ann (but not if they're the same ann)
    } else if ((policy == ROVPPAS_TYPE_ROVPP ||
                policy == ROVPPAS_TYPE_ROVPPB ||
                policy == ROVPPAS_TYPE_ROVPPBP ||
                policy == ROVPPAS_TYPE_ROVPPBIS) &&
               (relationship == (search->second.priority / 100) && ann != search->second)) {
        // Instead of tiebreak, we should pick the safest
        // Evaluate how safe are the two announcements
        std::set<Announcement> safe_announcements;
        Announcement curr_ann = search->second;
        // Is the curr_ann safe?
        bool is_curr_ann_safe  = true;
        for (const auto &curr_bad_ann : *failed_rov) {
            if (curr_bad_ann.prefix.contained_in_or_equal_to(curr_ann.prefix) &&
                curr_bad_ann.received_from_asn == curr_ann.received_from_asn) {
                is_curr_ann_safe  = false;
                break;
            }
        }
        bool is_new_ann_safe = true;
        for (const auto &curr_bad_ann : *failed_rov) {
            if (curr_bad_ann.prefix.contained_in_or_equal_to(ann.prefix) &&
                curr_bad_ann.received_from_asn == ann.received_from_asn) {
                is_new_ann_safe  = false;
                break;
            }
        }
        // Check which annoucements are safe
        // Is our current ann NOT safe and the new ann safe?    
        if (!is_curr_ann_safe && is_new_ann_safe) {
            // Replace our curr not safe announcement with the new safe announcement
            withdraw(search->second);
            search->second = ann;
            check_preventives(search->second);
        // Is our current ann safe and the new ann NOT safe?
        } if (is_curr_ann_safe && !is_new_ann_safe) {
            // Do nothing, because best announcement is in use 
        // If they're both equally safe, then pick the one with the shortest path (i.e. if new one has better path length)
        } else if (((is_curr_ann_safe && is_new_ann_safe) || (!is_curr_ann_safe && !is_new_ann_safe)) &&
                    ann.priority > search->second.priority) {
            // Replace our curr not safe announcement with the new safe announcement
            withdraw(search->second);
            search->second = ann;
            check_preventives(search->second);
        // TODO (review): If they're both equally safe, and path lengths are same, then pick deterministically randomly
        } else if (((is_curr_ann_safe && is_new_ann_safe) || (!is_curr_ann_safe && !is_new_ann_safe)) &&
                    ann.priority == search->second.priority) {
            // Random tiebraker
            //std::minstd_rand ran_bool(asn);
            ran = false;
            bool value = (ran ? get_random() : tiny_hash(ann.received_from_asn) < tiny_hash(search->second.received_from_asn) );
            // TODO This sets first come, first kept
            // value = false;
            if (value) {
                // Use the new announcement and record it won the tiebreak
                if (search_depref == depref_anns->end()) {
                    // Update inverse results
                    swap_inverse_result(
                        std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                        std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                    // Insert depref ann
                    depref_anns->insert(std::pair<Prefix<>, Announcement>(search->second.prefix, 
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
                // Use the old announcement
                if (search_depref == depref_anns->end()) {
                    depref_anns->insert(std::pair<Prefix<>, Announcement>(ann.prefix, 
                                                                          ann));
                } else {
                    // Replace second best with the old priority announcement
                    search_depref->second = ann;
                }
            }
        }
    // Otherwise check new announcements if relationship is better for best path selection
    } else if (ann.priority > search->second.priority) {
        if (search_depref == depref_anns->end()) {
            // Update inverse results
            swap_inverse_result(
                std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
            // Insert new second best announcement
            depref_anns->insert(std::pair<Prefix<>, Announcement>(search->second.prefix, 
                                                                  search->second));
            // Replace the old announcement with the higher priority
            withdraw(search->second);
            search->second = ann;
            check_preventives(search->second);
        } else {
            // Update inverse results
            swap_inverse_result(
                std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
            // Replace second best with the old priority announcement
            search_depref->second = search->second;
            // Replace the old announcement with the higher priority
            withdraw(search->second);
            search->second = ann;
            check_preventives(search->second);
        }
    // Old announcement was better
    } else {
        if (search_depref == depref_anns->end()) {
            // Insert new second best annoucement
            depref_anns->insert(std::pair<Prefix<>, Announcement>(ann.prefix, ann));
        } else if (ann.priority > search_depref->second.priority) {
            // Replace the old depref announcement with the higher priority
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
                        // Put the best alternative announcement into the loc_rib
                        Announcement best_alternative_ann = best_alternative_route(search->second); 
                        if (search->second != best_alternative_ann) {
                            search->second = best_alternative_ann;
                        } else {
                            loc_rib->erase(it->prefix);    
                        }
                        AS::graph_changed = true;  // This means we will need to do another propagation
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
        if (!pass_rovpp(ann)) {
            bad_neighbors->insert(ann.received_from_asn);
            failed_rov->insert(ann);
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
                // Put the best alternative announcement into the loc_rib
                Announcement best_alternative_ann = best_alternative_route(search->second); 
                if (search->second != best_alternative_ann) {
                    search->second = best_alternative_ann;
                } else {
                    loc_rib->erase(ann.prefix);    
                }
                AS::graph_changed = true;  // This means we will need to do another propagation
                
            }
            continue;
        }
        // If loc_rib announcement isn't a seeded announcement
        if (search == loc_rib->end() || !search->second.from_monitor) {
            // Regardless of policy, if the announcement originates from this AS
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
                    if (pass_rovpp(ann)) {
                        passed_rov->insert(ann);
                        process_announcement(ann, false);
                    }
                // ROV++ V0.1
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPP ||
                         policy_vector.at(0) == ROVPPAS_TYPE_ROVPP_LITE) {
                    // The policy for ROVpp 0.1 is similar to ROV in the extrapolator.
                    // Only in the data plane changes
                    if (pass_rovpp(ann)) {
                        passed_rov->insert(ann);
                        process_announcement(ann, false);
                    } else {
                        Announcement best_alternative_ann = best_alternative_route(ann); 
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
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPB ||
                           policy_vector.at(0) == ROVPPAS_TYPE_ROVPPB_LITE) {
                    // For ROVpp 0.2, forward a blackhole ann if there is no alt route.
                    if (pass_rovpp(ann)) {
                        passed_rov->insert(ann);
                        process_announcement(ann, false);
                    } else {
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // If no alternative
                            // Mark as blackholed and accept this announcement
                            blackholes->insert(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann, false);
                        } else {
                            process_announcement(best_alternative_ann, false);
                        }
                    }
                // New ROV++ V0.2bis (drops hijack announcements silently like v0.3)
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBIS || 
                         policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBIS_LITE) {
                    // For ROVpp 0.2bis, forward a blackhole ann to customers if there is no alt route.
                    if (pass_rovpp(ann)) {
                        passed_rov->insert(ann);
                        process_announcement(ann, false);
                    } else {
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // If no alternative
                            Announcement blackhole_ann = ann; 
                            // Mark as blackholed and accept this announcement
                            blackholes->insert(blackhole_ann);  // Make copy of annoucement to share as a blackhole ann
                            blackhole_ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            blackhole_ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(blackhole_ann, false);
                        } else {
                            process_announcement(best_alternative_ann, false);
                        }
                    }
                // ROV++ V0.3 (atually this is 3bis [or experiment to get rid of loops])
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP || 
                         policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP_LITE) {
                    // For ROVpp 0.3, forward a blackhole ann if there is no alt route.
                    // Also make a preventive announcement if there is an alt route.
                    if (pass_rovpp(ann)) {
                        passed_rov->insert(ann);
                        // For passed_rov announcements, we need to add a flag that indicates if the 
                        // attacking announcement was seen from the neighbor that shared this 
                        // passed_rov announcement. In which ROVppv3 would not select such announcement
                        // as an alternative route to issue a preventive announcement with.
                        if (bad_neighbors->find(ann.received_from_asn) != bad_neighbors->end()) {
                            ann.alt = ATTACKER_ON_ROUTE_FLAG;
                        }
                        process_announcement(ann, false);
                    } else {
                        // If it is from a customer, silently drop it
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_route(ann) == ann) { // If no alternative
                            Announcement blackhole_ann = ann;  // Make copy of annoucement to share as a blackhole ann
                            // Mark as blackholed and accept this announcement
                            blackholes->insert(blackhole_ann);
                            blackhole_ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            blackhole_ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(blackhole_ann);
                        } else {
                            // Make preventive announcement
                            Announcement preventive_ann = best_alternative_ann;
                            preventive_ann.prefix = ann.prefix;
                            preventive_ann.alt = best_alternative_ann.received_from_asn;
                            if (preventive_ann.origin == asn) { preventive_ann.received_from_asn=64514; }
                            preventive_anns->insert(std::pair<Announcement,Announcement>(preventive_ann, best_alternative_ann));
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
 * announcement it was given.
 * 
 * @param  ann An announcemnt you want to find an alternative for.
 * @return     The best alternative announcement (i.e. an announcement which came from a neighbor who hadn't shared
 *             an attacker announcemnt with us).
 */
Announcement ROVppAS::best_alternative_route(Announcement &ann) {
    // Initialize the default answer of (No best alternative with the current given ann)
    // This variable will update with the best ann if it exists
    Announcement best_alternative_ann = ann;
    // Create an ultimate list of good candidate announcemnts (ribs_in)
    std::vector<Announcement> candidates;
    std::set<Announcement> bad_ases_tmp = *failed_rov;
    uint32_t policy = policy_vector.at(0);
    // For each possible alternative
    // For LITE Versions
    if (policy == ROVPPAS_TYPE_ROVPP_LITE || 
        policy == ROVPPAS_TYPE_ROVPPB_LITE ||
        policy == ROVPPAS_TYPE_ROVPPBP_LITE ||
        policy == ROVPPAS_TYPE_ROVPPBIS_LITE) {
       // Check if the local rib announcement we currently have is still good (if we have one)
       // 1. Search local rib for good announcement
       for (auto candidate_ann_pair : *loc_rib) {
           auto candidate_ann = candidate_ann_pair.second;
           // 2. Is our good announcement received_from_asn the same as the attacker ann
           // Check if the candidate announcement comes from the same neighbor from which we received the attacker announcement from
           if (candidate_ann.received_from_asn == ann.received_from_asn && 
                (ann.prefix.contained_in_or_equal_to(candidate_ann.prefix) || candidate_ann.prefix.contained_in_or_equal_to(ann.prefix))) {
                return ann;
           } else {
                if (policy == ROVPPAS_TYPE_ROVPPBP_LITE) {
                    if (candidate_ann.alt != ATTACKER_ON_ROUTE_FLAG) {
                        candidates.push_back(candidate_ann);
                    }
                } else {
                    candidates.push_back(candidate_ann);
                }
           }
       }
       // Since we preocessed all the candidates above, picking the first on the list here is appropriate
       // Any candidate announcement is acceptable (it's already in the loc_rib, and we don't want to modify the loc_rib)
       if (candidates.size() != 0) {    
            return candidates.at(0);
       } else {
            return ann;
       }
    }
    
    // NOTE:
    // The rest of the function only works for Non_LITE versions

    // For Non_LITE Versions
    else { 
        for (auto candidate_ann : *ribs_in) {
            if (pass_rovpp(candidate_ann) && !candidate_ann.withdraw) {
                if (policy == ROVPPAS_TYPE_ROVPPBP) {
                    if (candidate_ann.alt != ATTACKER_ON_ROUTE_FLAG) {
                        candidates.push_back(candidate_ann);
                    }
                } else {
                    candidates.push_back(candidate_ann);
                }
            } else {
              bad_ases_tmp.insert(candidate_ann);
            }
        }
    }
    // Find the best alternative to ann
    for (auto &candidate : candidates) {
      // Is there a valid alternative?
      if (ann.prefix.contained_in_or_equal_to(candidate.prefix)) {
         // Is the candidate safe?
         bool safe = true;
         for (auto &curr_bad_ann : bad_ases_tmp) {
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
             } else if (
                (best_alternative_ann.prefix.netmask < candidate.prefix.netmask) ||
                (best_alternative_ann.priority < candidate.priority && best_alternative_ann.prefix == candidate.prefix)
             ) {
                 best_alternative_ann = candidate;
             }
         }
      }
    }
    return best_alternative_ann;
}

void ROVppAS::check_preventives(Announcement ann) {
    // ROV++ V0.3
    if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP || policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP_LITE) {
        // note this only works for /24...
        if (ann.prefix.netmask == 0xffffff00) {
            // this is already a preventive
            return;
        }
        ann.prefix.netmask = 0xffffff00;
        // find the preventive ann if it exists
        auto search = loc_rib->find(ann.prefix);
        if (search != loc_rib->end()) {
            Announcement best_alternative_ann = best_alternative_route(search->second); 
            if (ann.received_from_asn == search->second.received_from_asn) {
                // Remove and attempt to replace the preventive ann
                withdrawals->push_back(ann);
                loc_rib->erase(ann.prefix);    
                ann.withdraw = false;
                // replace
                if (best_alternative_route(ann) == ann) { // If no alternative
                    // Mark as blackholed and accept this announcement
                    blackholes->insert(ann);
                    ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                    ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                    process_announcement(ann);
                } else {
                    // Make preventive announcement
                    Announcement preventive_ann = best_alternative_ann;
                    preventive_ann.prefix = ann.prefix;
                    preventive_ann.alt = best_alternative_ann.received_from_asn;
                    if (preventive_ann.origin == asn) { preventive_ann.received_from_asn=64514; }
                    preventive_anns->insert(std::pair<Announcement,Announcement>(preventive_ann, best_alternative_ann));
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
        value = (value ^ mask & (as_number>>i)) * 3;
    }
    return value;
}

/**
 * [ROVppAS::stream_blacklist description]
 * @param  os [description]
 * @return    [description]
 */
std::ostream& ROVppAS::stream_blackholes(std:: ostream &os) {
  for (Announcement ann : *blackholes) {
      os << asn << ",";
      ann.to_blackholes_csv(os);
  }
  return os;
}
