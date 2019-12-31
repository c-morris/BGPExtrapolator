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
                 std::set<uint32_t> *cust): 
                 AS(myasn, inv, prov, peer, cust)  {
                    // Save reference to attackers
                    attackers = rovpp_attackers;
                    failed_rov = new std::vector<Announcement>;
                    passed_rov = new std::vector<Announcement>;
                    blackholes = new std::vector<Announcement>;
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

/**
 * Checks whether or not an announcement is from an attacker
 * 
 * @param  ann [description]
 * @return bool  true if from attacker, false otherwise
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
                    if (best_alternative_route(ann) == ann) { // if no alternative
                        // mark as blackholed and accept this announcement
                        blackholes->push_back(ann);
                        ann.origin = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                        ann.received_from_asn = UNUSED_ASN_FLAG_FOR_BLACKHOLES;
                        process_announcement(ann);
                    } // else drop it
                }
            } else { // unrecognized policy defaults to bgp
                process_announcement(ann);
            }
        } else { // if there is no policy
            process_announcement(ann);
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
     std::vector<Announcement> candidates;
     candidates.reserve(passed_rov->size() + incoming_announcements->size());  // preallocate memory
     candidates.insert(candidates.end(), passed_rov->begin(), passed_rov->end());
     for (auto candidate_ann : *incoming_announcements) {
         if (pass_rov(candidate_ann)) {
             candidates.push_back(candidate_ann);
         }
     }
     // Check if the prefix is in our history
     for(std::size_t i=0; i<candidates.size(); ++i) {
         Announcement curr_good_ann = candidates.at(i);
         for(std::size_t j=0; j<failed_rov->size(); ++j) {
             Announcement curr_bad_ann = failed_rov->at(j);
             // Check if the prefixes overlap or match
             // and if they DO NOT come from the same neighbor
             if ((curr_bad_ann.prefix.contained_in_or_equal_to(curr_good_ann.prefix)) && 
                 (curr_bad_ann.received_from_asn != curr_good_ann.received_from_asn)) {
                 // Update the best_alternative_ann if it's better than the current one
                 if (is_better(curr_good_ann, best_alternative_ann)) {
                     best_alternative_ann = curr_good_ann;
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

