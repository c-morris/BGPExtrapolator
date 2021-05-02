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

    if(depref_anns != NULL)
        delete depref_anns;
    
    delete peers;
    delete providers;
    delete customers;
    delete member_ases;
}

template <class AnnouncementType>
uint8_t BaseAS<AnnouncementType>::tiny_hash(uint32_t as_number) {
    uint8_t mask = 0xFF;
    uint8_t value = 0;
    for (size_t i = 0; i < sizeof(as_number); i++) {
        value = (value ^ (mask & (as_number>>(i * 8)))) * 3;
    }
    return value;
}

template <class AnnouncementType>
bool BaseAS<AnnouncementType>::get_random() {
    bool r = (tiny_hash(asn) % 2 == 0);
    return r;
}

//****************** Relationship Handling ******************//

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

//****************** Announcement Handling ******************//

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

template <class AnnouncementType>
void BaseAS<AnnouncementType>::receive_announcements(std::vector<AnnouncementType> &announcements) {
    for (AnnouncementType &ann : announcements) {
        // push_back makes a copy of the announcement
        incoming_announcements->push_back(ann);
    }
}

template <class AnnouncementType>
void BaseAS<AnnouncementType>::process_announcement(AnnouncementType &ann, bool ran) {
    // Check for existing announcement for prefix
    auto search = all_anns->find(ann.prefix);
    
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
                // Update inverse results
                if(inverse_results != NULL) {
                    swap_inverse_result(
                        std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                        std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                }

                // Use the new announcement
                if(depref_anns != NULL) {
                    auto search_depref = depref_anns->find(ann.prefix);
                    if (search_depref == depref_anns->end())
                        // Insert depref ann
                        depref_anns->insert(std::pair<Prefix<>, AnnouncementType>(search->second.prefix, search->second));
                    else
                        search_depref->second = search->second;
                }

                search->second = ann;
            } else if(depref_anns != NULL) {
                auto search_depref = depref_anns->find(ann.prefix);

                // Use the old announcement
                if (search_depref == depref_anns->end()) {
                    // Insert new second best announcement
                    depref_anns->insert(std::pair<Prefix<>, AnnouncementType>(ann.prefix, ann));
                } else {
                    // Replace second best with the old priority announcement
                    search_depref->second = ann;
                }
            }
        // Otherwise check new announcements priority for best path selection
        } else if (ann.priority > search->second.priority) {
            if(inverse_results != NULL) {
                // Update inverse results
                swap_inverse_result(
                    std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                    std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
            }

            if(depref_anns != NULL) {
                auto search_depref = depref_anns->find(ann.prefix);
                if (search_depref == depref_anns->end()) {
                    // Insert new second best announcement
                    depref_anns->insert(std::pair<Prefix<>, AnnouncementType>(search->second.prefix, search->second));
                } else {
                    // Replace second best with the old priority announcement
                    search_depref->second = search->second;
                }
            }

            // Replace the old announcement with the higher priority
            search->second = ann;
        // Old announcement was better
        // Check depref announcements priority for best path selection
        } else if(depref_anns != NULL) {
            auto search_depref = depref_anns->find(ann.prefix);
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

template <class AnnouncementType>
void BaseAS<AnnouncementType>::clear_announcements() {
    all_anns->clear();
    incoming_announcements->clear();

    if(depref_anns != NULL)
        depref_anns->clear();
}

template <class AnnouncementType>
bool BaseAS<AnnouncementType>::already_received(AnnouncementType &ann) {
    auto search = all_anns->find(ann.prefix);
    return !(search == all_anns->end());
}

template <class AnnouncementType>
void BaseAS<AnnouncementType>::delete_ann(AnnouncementType &ann) {
    all_anns->erase(ann.prefix);
}

template <class AnnouncementType>
void BaseAS<AnnouncementType>::delete_ann(Prefix<> &prefix) {
    all_anns->erase(prefix);
}

//****************** FILE I/O ******************//

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

template <class AnnouncementType>
std::ostream& BaseAS<AnnouncementType>::stream_announcements(std::ostream &os) {
    for (auto &ann : *all_anns) {
        os << asn << ',';
        ann.second.to_csv(os);
    }
    return os;
}

template <class AnnouncementType>
std::ostream& BaseAS<AnnouncementType>::stream_depref(std::ostream &os) {
    if(depref_anns != NULL) {
        for (auto &ann : *depref_anns) {
            os << asn << ',';
            ann.second.to_csv(os);
        }
    }
    return os;
}

//We love C++
template class BaseAS<Announcement>;
template class BaseAS<EZAnnouncement>;
template class BaseAS<ROVppAnnouncement>;
template class BaseAS<ROVAnnouncement>;
