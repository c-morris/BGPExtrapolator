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
#include "ASes/BaseAS.h"

template <class AnnouncementType>
BaseAS<AnnouncementType>::~BaseAS() {
    delete incoming_announcements;
    delete all_anns;
    delete depref_anns;
    delete peers;
    delete providers;
    delete customers;
    delete member_ases;
}


/** Generates a random boolean value.
 */
template <class AnnouncementType>
bool BaseAS<AnnouncementType>::get_random() {
    bool r = (ran_bool() % 2 == 0);
    return r;
}

/** Add neighbor AS to the appropriate set in this AS based on the relationship.
 *
 * @param asn ASN of neighbor.
 * @param relationship AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
 */
template <class AnnouncementType>
void BaseAS<AnnouncementType>::add_neighbor(uint32_t asn, int relationship) {
    switch (relationship) {
        case AS_REL_PROVIDER:
            providers->insert(asn);
            break;
        case AS_REL_PEER:
            peers->insert(asn);
            break;
        case AS_REL_CUSTOMER:
            customers->insert(asn);
            break;
    }
}

/** Remove neighbor AS from the appropriate set in this AS based on the relationship.
 *
 * @param asn ASN of neighbor.
 * @param relationship AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
 */
template <class AnnouncementType>
void BaseAS<AnnouncementType>::remove_neighbor(uint32_t asn, int relationship) {
    switch (relationship) {
        case AS_REL_PROVIDER:
            providers->erase(asn);
            break;
        case AS_REL_PEER:
            peers->erase(asn);
            break;
        case AS_REL_CUSTOMER:
            customers->erase(asn);
            break;
    }
}

/** Swap a pair of prefix/origins for this AS in the inverse results.
 *
 * @param old The prefix/origin to be inserted
 * @param current The prefix/origin to be removed
 */
template <class AnnouncementType>
void BaseAS<AnnouncementType>::swap_inverse_result(std::pair<Prefix<>,uint32_t> old, std::pair<Prefix<>,uint32_t> current) {
    if (inverse_results != NULL) {
        // Add back to old set, remove from new set
        auto set = inverse_results->find(old);
        if (set != inverse_results->end()) {
            set->second->insert(asn);
        }
        set = inverse_results->find(current);
        if (set != inverse_results->end()) {
            set->second->erase(asn);
        }
    }
}

/** Push the incoming propagated announcements to the incoming_announcements vector.
 *
 * This is NOT called for seeded announcements.
 *
 * @param announcements The announcements to be pushed onto the incoming_announcements vector.
 */
template <class AnnouncementType>
void BaseAS<AnnouncementType>::receive_announcements(std::vector<AnnouncementType> &announcements) {
    for (AnnouncementType &ann : announcements) {
        // push_back makes a copy of the announcement
        incoming_announcements->push_back(ann);
    }
}

/** Processes a single announcement, adding it to the ASes set of announcements if appropriate.
 *
 * Approximates BGP best path selection based on announcement priority.
 * Called by process_announcements and Extrapolator.give_ann_to_as_path()
 * 
 * @param ann The announcement to be processed
 */ 
template <class AnnouncementType>
void BaseAS<AnnouncementType>::process_announcement(AnnouncementType &ann, bool ran) {
    // Check for existing announcement for prefix
    auto search = all_anns->find(ann.prefix);
    auto search_depref = depref_anns->find(ann.prefix);
    
    // No announcement found for incoming announcement prefix
    if (search == all_anns->end()) {
        all_anns->insert(std::pair<Prefix<>, AnnouncementType>(ann.prefix, ann));
        // Inverse results need to be computed also with announcements from monitors
        if (inverse_results != NULL) {
            auto set = inverse_results->find(
                std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
            if (set != inverse_results->end()) {
                set->second->erase(asn);
            }
        }
    } else {
        // Logger::getInstance().log("Matching_Prefixes") << "Received an additional announcement for prefix:" << ann.prefix.to_cidr() << ", tstamp on processing announcement: " 
        //             << ann.tstamp << ", timestamp on stored announcement: " << search->second.tstamp
        //             << ", origin on processing announcement: " << ann.origin << ", origin on stored announcement: " << search->second.origin;

        // Tiebraker for equal priority between old and new ann
        if (ann.priority == search->second.priority) { 
            // Tiebreaker
            bool value = true;
            // Random tiebreaker if enabled
            if (ran) {
                value = get_random();
            }

            // Logger::getInstance().log("Equal_Priority") << "Equal Priority announcements on prefix: " << ann.prefix.to_cidr() << 
            //         ", rand value: " << value << ", tstamp on processing announcement: " << ann.tstamp << ", timestamp on stored announcement: " << search->second.tstamp
            //         << ", origin on processing announcement: " << ann.origin << ", origin on stored announcement: " << search->second.origin;

            // Defaults to first come, first kept if not random
            if (value) {
                // Use the new announcement
                if (search_depref == depref_anns->end()) {
                    // Update inverse results
                    swap_inverse_result(
                        std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                        std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                    // Insert depref ann
                    depref_anns->insert(std::pair<Prefix<>, AnnouncementType>(search->second.prefix, 
                                                                        search->second));
                    search->second = ann;
                } else {
                    swap_inverse_result(
                        std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                        std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                    search_depref->second = search->second;
                    search->second = ann;
                }
            } else {
                // Use the old announcement
                if (search_depref == depref_anns->end()) {
                    depref_anns->insert(std::pair<Prefix<>, AnnouncementType>(ann.prefix, 
                                                                        ann));
                } else {
                    // Replace second best with the old priority announcement
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
                // Insert new second best announcement
                depref_anns->insert(std::pair<Prefix<>, AnnouncementType>(search->second.prefix, 
                                                                    search->second));
                // Replace the old announcement with the higher priority
                search->second = ann;
            } else {
                // Update inverse results
                swap_inverse_result(
                    std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                    std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                // Replace second best with the old priority announcement
                search_depref->second = search->second;
                // Replace the old announcement with the higher priority
                search->second = ann;
            }
        // Old announcement was better
        // Check depref announcements priority for best path selection
        } else {
            if (search_depref == depref_anns->end()) {
                // Insert new second best annoucement
                depref_anns->insert(std::pair<Prefix<>, AnnouncementType>(ann.prefix, ann));
            } else if (ann.priority > search_depref->second.priority) {
                // Replace the old depref announcement with the higher priority
                search_depref->second = search->second;
            }
        }
    }
}

/** Iterate through incoming_announcements and keep only the best. 
 */
template <class AnnouncementType>
void BaseAS<AnnouncementType>::process_announcements(bool ran) {
    for (auto &ann : *incoming_announcements) {
        auto search = all_anns->find(ann.prefix);
        if (search == all_anns->end() || !search->second.from_monitor) {
            process_announcement(ann, ran);
        }
    }
    incoming_announcements->clear();
}

/** Clear all announcement collections. 
 */
template <class AnnouncementType>
void BaseAS<AnnouncementType>::clear_announcements() {
    all_anns->clear();
    incoming_announcements->clear();
    depref_anns->clear();
}

/** Check if a monitor announcement is already recv'd by this AS. 
 *
 * @param ann Announcement to check for. 
 * @return True if recv'd, false otherwise.
 */
template <class AnnouncementType>
bool BaseAS<AnnouncementType>::already_received(AnnouncementType &ann) {
    auto search = all_anns->find(ann.prefix);
    bool found = (search == all_anns->end()) ? false : true;
    return found;
}

/** Deletes given announcement.
 */
template <class AnnouncementType>
void BaseAS<AnnouncementType>::delete_ann(AnnouncementType &ann) {
    all_anns->erase(ann.prefix);
}

/** Insertion operator for AS class.
 *
 * @param os
 * @param as
 * @return os passed as parameter
 */
template <class U>
std::ostream& operator<<(std::ostream &os, const BaseAS<U>& as) {
    os << "ASN: " << as.asn << std::endl << "Rank: " << as.rank
        << std::endl << "Providers: ";
    for (auto &provider : *as.providers) {
        os << provider << ' ';
    }
    os << '\n';
    os << "Peers: ";
    for (auto &peer : *as.peers) {
        os << peer << ' ';
    }
    os << '\n';
    os << "Customers: ";
    for (auto &customer : *as.customers) {
        os << customer << ' ';
    }
    os << '\n';
    return os;
}

/** Streams announcements to an output stream in a .csv readable file format.
 *
 * @param os
 * @return output stream into which is passed the .csv row formatted announcements
 */
template <class AnnouncementType>
std::ostream& BaseAS<AnnouncementType>::stream_announcements(std::ostream &os) {
    for (auto &ann : *all_anns) {
        os << asn << ',';
        ann.second.to_csv(os);
    }
    return os;
}

/** Streams depref announcements to an output stream in a .csv readable file format.
 *
 * @param os
 * @return output stream into which is passed the .csv row formatted announcements
 */
template <class AnnouncementType>
std::ostream& BaseAS<AnnouncementType>::stream_depref(std::ostream &os) {
    for (auto &ann : *depref_anns) {
        os << asn << ',';
        ann.second.to_csv(os);
    }
    return os;
}

template class BaseAS<Announcement>;
template class BaseAS<EZAnnouncement>;
template class BaseAS<ROVppAnnouncement>;