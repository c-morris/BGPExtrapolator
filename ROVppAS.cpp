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
    failed_rov = new std::vector<Announcement>;
    passed_rov = new std::vector<Announcement>;
    blackholes = new std::vector<Announcement>;
    preventive_anns = new std::vector<std::pair<Announcement,Announcement>>;  
}

ROVppAS::~ROVppAS() { 
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

/** Checks whether or not an announcement is from an attacker
 * 
 * @param  ann  announcement to check if it passes ROV
 * @return bool  return false if from attacker, true otherwise
 */
bool ROVppAS::pass_rov(Announcement &ann) {
    if (ann.origin == UNUSED_ASN_FLAG_FOR_BLACKHOLES) { return false; }
    if (attackers != NULL) {
        return (attackers->find(ann.origin) == attackers->end());
    } else {
        return true;
    }
}

void ROVppAS::withdraw(Announcement &ann) {
    Announcement copy = ann;
    copy.withdraw = true;
    ribs_out->push_back(copy);
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
    
    // ROV++ 
    // Check preventive announcement alt
    if (ann.alt == asn) {
        // Ignore the preventive announcement
        return;
    }

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
    // Tiebraker for equal priority between old and new ann
    } else if (ann.priority == search->second.priority) {
        // Check for override
        if (search->second.received_from_asn == ann.tiebreak_override) {
            search->second = ann;
            withdraw(ann);
        } else {
            // Random tiebraker
            //std::minstd_rand ran_bool(asn);
            bool value = (ran ? get_random() : ann.received_from_asn < search->second.received_from_asn );
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
                    ann.tiebreak_override = ann.received_from_asn;
                    search->second = ann;
                    withdraw(ann);
                } else {
                    swap_inverse_result(
                        std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                        std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                    search_depref->second = search->second;
                    ann.tiebreak_override = ann.received_from_asn;
                    search->second = ann;
                    withdraw(ann);
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
    // Otherwise check new announcements priority for best path selection
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
            search->second = ann;
            withdraw(ann);
        } else {
            // Update inverse results
            swap_inverse_result(
                std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
            // Replace second best with the old priority announcement
            search_depref->second = search->second;
            // Replace the old announcement with the higher priority
            search->second = ann;
            withdraw(ann);
        }
    // Old announcement was better
    // Check depref announcements priority for best path selection
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
    for (auto &ann : *ribs_in) {
        auto search = loc_rib->find(ann.prefix);
        if (search == loc_rib->end() || !search->second.from_monitor) {
            // Regardless of policy, if the announcement originates from this AS
            // *or is a subprefix of its own prefix*
            // the received_from_asn set to 64514 (if we are not an attacker)
            if (ann.origin == asn && attackers->find(asn) == attackers->end()) { ann.received_from_asn=64514; }
            // Process withdrawals, regardless of policy
            if (ann.withdraw) {
                if (search != loc_rib->end()) {
                    loc_rib->erase(ann.prefix);    
                }
                ribs_out->push_back(ann);
                continue;
            }
            for (auto rib_ann : *loc_rib) {
                if (ann.prefix.contained_in_or_equal_to(rib_ann.second.prefix) &&
                    rib_ann.second.origin == asn &&
                    attackers->find(asn) == attackers->end()) {
                    ann.received_from_asn=64514;
                }
            }
            if (policy_vector.size() > 0) { // if we have a policy
                if (policy_vector.at(0) == ROVPPAS_TYPE_ROV) {
                    if (pass_rov(ann)) {
                        process_announcement(ann, ran);
                    }
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPP) {
                    // The policy for ROVpp 0.1 is identical to ROV in the extrapolator.
                    // Only in the data plane changes
                    if (pass_rov(ann)) {
                        passed_rov->push_back(ann);
                        process_announcement(ann, ran);
                    } else {
                        failed_rov->push_back(ann);
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // if no alternative
                            blackholes->push_back(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann, ran);
                        } else {
                            process_announcement(best_alternative_ann, ran);
                        }
                    }
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPB) {
                    // For ROVpp 0.2, forward a blackhole ann if there is no alt route.
                    if (pass_rov(ann)) {
                        passed_rov->push_back(ann);
                        process_announcement(ann, ran);
                    } else {
                        failed_rov->push_back(ann);
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // if no alternative
                            // mark as blackholed and accept this announcement
                            blackholes->push_back(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann, ran);
                        } else {
                          process_announcement(best_alternative_ann, ran);
                        }
                    }
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBIS) {
                    // For ROVpp 0.2bis, forward a blackhole ann to customers if there is no alt route.
                    if (pass_rov(ann)) {
                        passed_rov->push_back(ann);
                        process_announcement(ann, ran);
                    } else {
                        failed_rov->push_back(ann);
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // if no alternative
                            // mark as blackholed and accept this announcement
                            blackholes->push_back(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann, ran);
                        } else {
                          process_announcement(best_alternative_ann, ran);
                        }
                    }
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPBP) {
                    // For ROVpp 0.3, forward a blackhole ann if there is no alt route.
                    // Also make a preventive announcement if there is an alt route.
                    if (pass_rov(ann)) {
                        passed_rov->push_back(ann);
                        process_announcement(ann);
                    } else {
                        // if it is from a customer, silently drop it
                        if (customers->find(ann.received_from_asn) != customers->end()) { continue; }
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        failed_rov->push_back(ann);
                        if (best_alternative_route(ann) == ann) { // if no alternative
                            // mark as blackholed and accept this announcement
                            blackholes->push_back(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann);
                        } else {
                            // Make preventive announcement
                            Announcement preventive_ann = best_alternative_ann;
                            preventive_ann.prefix = ann.prefix;
                            preventive_ann.alt = best_alternative_ann.received_from_asn;
                            if (preventive_ann.origin == asn) { preventive_ann.received_from_asn=64514; }
                            preventive_anns->push_back(std::pair<Announcement,Announcement>(preventive_ann, best_alternative_ann));
                            process_announcement(preventive_ann);
                        }
                    }
                } else { // unrecognized policy defaults to bgp
                    process_announcement(ann, ran);
                }
            } else { // if there is no policy
                process_announcement(ann, ran);
            }
        }
    }
    //ribs_in->clear();
}

/**
 * Will return the best alternative announcemnt if it exists. If it doesn't exist, it will return the
 * the announcement it was given.
 * 
 * @param  ann An announcemnt you want to find an alternative for.
 * @return     The best alternative announcement (i.e. an announcement which came from a neighbor who hadn't shared
 *             an attacker announcemnt with us).
 */
 Announcement ROVppAS::best_alternative_route(Announcement &ann) {
     // Initialize the default answer of (No best alternative with the current given ann)
     // This variable will update with the best ann if it exists
     Announcement best_alternative_ann = ann;
     // Create an ultimate list of good candidate announcemnts (passed_rov + ribs_in)
     std::vector<Announcement> candidates = *passed_rov;
     std::vector<Announcement> baddies = *failed_rov;
     for (auto candidate_ann : *ribs_in) {
         if (pass_rov(candidate_ann)) {
             candidates.push_back(candidate_ann);
         } else {
             baddies.push_back(candidate_ann);
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

/**
 * Computes "Is a better than b?".
 * @param  a Announcement 1
 * @param  b Announcement 2
 * @return   Will return a bool of whether or not a is better than b
 */
bool ROVppAS::is_better(Announcement &a, Announcement &b) {    
    // TODO: We will return to this to consider blackhole size
    // We opted to use this for simplicity, but the case of considering
    // whether or not a blackhole exists in one of these two ann is significant
    // Use BGP priority to make decision
    return  a.priority > b.priority;
}

/**
 * [ROVppAS::stream_blacklist description]
 * @param  os [description]
 * @return    [description]
 */
std::ostream& ROVppAS::stream_blackholes(std:: ostream &os) {
  for(std::size_t i=0; i<blackholes->size(); ++i) {
      Announcement ann = blackholes->at(i);
      os << asn << ",";
      ann.to_blackholes_csv(os);
  }
  return os;
}

