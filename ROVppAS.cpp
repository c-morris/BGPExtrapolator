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
                 }
}

ROVppAS::~ROVppAS() { 
    delete failed_rov;
    delete passed_rov;
    delete blackholes;
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

/** Iterate through incoming_announcements and keep only the best. 
 */
void ROVppAS::process_announcements() {
    for (auto &ann : *incoming_announcements) {
        auto search = all_anns->find(ann.prefix);
        if (search == all_anns->end() || !search->second.from_monitor) {
            if (policy_vector.size() > 0) { // if we have a policy
                if (policy_vector.at(0) == ROVPPAS_TYPE_ROV) {
                    if (pass_rov(ann)) {
                        process_announcement(ann);
                    }
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPP) {
                    // The policy for ROVpp 0.1 is identical to ROV in the extrapolator.
                    // Only in the data plane changes
                    if (pass_rov(ann)) {
                        passed_rov->push_back(ann);
                        process_announcement(ann);
                    } else {
                        failed_rov->push_back(ann);
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // if no alternative
                            blackholes->push_back(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann);
                        } else {
                            process_announcement(best_alternative_ann);
                        }
                    }
                } else if (policy_vector.at(0) == ROVPPAS_TYPE_ROVPPB) {
                    // For ROVpp 0.2, forward a blackhole ann if there is no alt route.
                    if (pass_rov(ann)) {
                        passed_rov->push_back(ann);
                        process_announcement(ann);
                    } else {
                        failed_rov->push_back(ann);
                        Announcement best_alternative_ann = best_alternative_route(ann); 
                        if (best_alternative_ann == ann) { // if no alternative
                            // mark as blackholed and accept this announcement
                            blackholes->push_back(ann);
                            ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                            process_announcement(ann);
                        } else {
                          process_announcement(best_alternative_ann);
                        }
                    }
                } else { // unrecognized policy defaults to bgp
                    process_announcement(ann);
                }
            } else { // if there is no policy
                process_announcement(ann);
            }
        }
    }
    incoming_announcements->clear();
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
     // Create an ultimate list of good candidate announcemnts (passed_rov + incoming_announcements)
     std::vector<Announcement> candidates = *passed_rov;
     std::vector<Announcement> baddies = *failed_rov;
     for (auto candidate_ann : *incoming_announcements) {
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
      ann.to_csv(os);
  }
  return os;
}

