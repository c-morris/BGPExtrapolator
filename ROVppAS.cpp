#include <cstdint>
#include "ROVppAS.h"
#include "NegativeAnnouncement.h"
#include "Prefix.h"


ROVppAS::~ROVppAS() {
    delete incoming_announcements;
    delete anns_sent_to_peers_providers;
    delete all_anns;
    delete peers;
    delete providers;
    delete customers;
    delete member_ases;
    delete as_graph;
}


bool ROVppAS::pass_rov(Announcement &ann) {
  if (ann.origin == attacker_asn) {
    return false;
  } else {
    return true;
  }
}


void ROVppAS::incoming_negative_announcement(Announcement &ann) {
  // Check if you have an alternative route (i.e. escape route)
  std::pair<bool, Announcement*> check = received_valid_announcement(ann);
  if (check.first) {
    // Check if the attaker's path and this path intersect
    // MARK: Can this be done in practice?
    // TODO: ann should be the hijacked announcement (modify NegativeAnnouncement to save the HijackedAnn)
    if (paths_intersect(*(check.second), ann)) {
      make_negative_announcement_and_blackhole(ann, *(check.second));
    }
  } else {  // You don't have an escapce route
    make_negative_announcement_and_blackhole(ann, *(check.second));
  }
}


void ROVppAS::incoming_valid_announcement(Announcement &ann) {
  // Do not check for duplicates here
  // push_back makes a copy of the announcement
  incoming_announcements->push_back(ann);

  // Check if should make negative announcement
  // Check if you have received the Invalid Announcement in the past
  std::pair<bool, Announcement*> check = received_hijack_announcement(ann);
  if (check.first) {
    // Check if the a ttaker's path and this path intersect
    // MARK: Can this be done in practice?
    if (paths_intersect(ann, *(check.second))) {
      make_negative_announcement_and_blackhole(ann, *(check.second));
    } else {  // If already blocked this in the past, then you need to unblock and unblackhole
      // Remove from dropped list
      dropped_ann_map.erase(check.second->prefix);
      // Remove from blackhole list
      blackhole_map.erase(check.second->prefix);
    }
  }
}


void ROVppAS::incoming_hijack_announcement(Announcement &ann) {
  // Drop the announcement (i.e. don't add it to incoming_announcements)
  // Add announcement to dropped list
  dropped_ann_map[ann.prefix] = ann;
  // Check if should make negative announcement
  // Check if you have received the Valid announcement in the past
  std::pair<bool, Announcement*> check = received_valid_announcement(ann);
  if (check.first) {
    // Also check if the attaker's path and this path intersect
    if (paths_intersect(*(check.second), ann)) {
      make_negative_announcement_and_blackhole(*(check.second), ann);
    }
  }
}


/** Push the received announcements to the incoming_announcements vector.
 *
 * Note that this differs from the Python version in that it does not store
 * a dict of (prefix -> list of announcements for that prefix).
 *
 * @param announcements to be pushed onto the incoming_announcements vector.
 */
void ROVppAS::receive_announcements(std::vector<Announcement> &announcements) {
  for (Announcement &ann : announcements) {
    // Check if it's a NegativeAnnouncement
    if (NegativeAnnouncement* d = dynamic_cast<NegativeAnnouncement*>(&ann)) {
      incoming_negative_announcement(*d);
    } else if (pass_rov(ann)) { // It's not a negative announcement. Check if the Announcement is valid
      incoming_valid_announcement(ann);
    } else {  // Not valid (i.e. hijack announcement)
      incoming_hijack_announcement(ann);
    }
  }
}


/**
 *
 * may need to put in incoming_announcements for speed
 * called by ASGraph.give_ann_to_as_path()
 */
void ROVppAS::receive_announcement(Announcement &ann) {
    // Check for existing annoucement for prefix
    auto search = all_anns->find(ann.prefix);
    auto search_alt = depref_anns->find(ann.prefix);

    // No announcement found for incoming announcement prefix
    if (search == all_anns->end()) {
        all_anns->insert(std::pair<Prefix<>, Announcement>(ann.prefix, ann));
        // Inverse results need to be computed also with announcements from monitors
        if (inverse_results != NULL) {
            auto set = inverse_results->find(
                std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
            if (set != inverse_results->end()) {
                set->second->erase(asn);
            }
        }

    // Tiebraker for equal priority
    } else if (ann.priority == search->second.priority) {
        // Default to lower ASN
        if (ann.received_from_asn < search->second.received_from_asn) {     // New ASN is lower
            if (search_alt == depref_anns->end()) {
                swap_inverse_result(
                    std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                    std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                depref_anns->insert(std::pair<Prefix<>, Announcement>(search->second.prefix,
                                                                      search->second));
                search->second = ann;
            } else {
                swap_inverse_result(
                    std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                    std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
                search_alt->second = search->second;
                search->second = ann;
            }
        } else {                                // Old ASN is lower
            if (search_alt == depref_anns->end()) {
                depref_anns->insert(std::pair<Prefix<>, Announcement>(ann.prefix,
                                                                      ann));
            } else {
                // Replace second best with the old priority announcement
                search_alt->second = ann;
            }
        }

    // Check announcements priority for best path selection
    } else if (ann.priority > search->second.priority) {
        if (search_alt == depref_anns->end()) {
            // update inverse results
            swap_inverse_result(
                std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
            // Insert new second best announcement
            depref_anns->insert(std::pair<Prefix<>, Announcement>(search->second.prefix,
                                                                  search->second));
            // Replace the old announcement with the higher priority
            search->second = ann;
        } else {
            // update inverse results
            swap_inverse_result(
                std::pair<Prefix<>, uint32_t>(search->second.prefix, search->second.origin),
                std::pair<Prefix<>, uint32_t>(ann.prefix, ann.origin));
            // Replace second best with the old priority announcement
            search_alt->second = search->second;
            // Replace the old announcement with the higher priority
            search->second = ann;
        }

    // Check second best announcements priority for best path selection
    } else {
        if (search_alt == depref_anns->end()) {
            // Insert new second best annoucement
            depref_anns->insert(std::pair<Prefix<>, Announcement>(ann.prefix, ann));
        } else if (ann.priority > search_alt->second.priority) {
            // Replace the old depref announcement with the higher priority
            search_alt->second = search->second;
        }
    }
}


/** Make negative announcement AND Blackhole's that announcement
*/
void ROVppAS::make_negative_announcement_and_blackhole(Announcement &legit_ann, Announcement &hijack_ann) {
  // Create Negative Announcement
  // TODO: Delete following block of code after testing
  // Prefix<> s("137.99.0.0", "255.255.255.0");
  // Prefix<> p("137.99.0.0", "255.255.0.0");
  // NegativeAnnouncement neg_ann = NegativeAnnouncement(13796, p.addr, p.netmask, 22742, std::set<Prefix<>>());
  // NegativeAnnouncement neg_ann = NegativeAnnouncement(uint32_t aorigin,
  //                                                     uint32_t aprefix,
  //                                                     uint32_t anetmask,
  //                                                     uint32_t from_asn,
  //                                                     std::set<Prefix<>> n_routed);
  std::cout << "Making Blackhole at AS" << asn << " for prefix: " << hijack_ann.prefix.to_cidr() << std::endl;
  NegativeAnnouncement neg_ann = NegativeAnnouncement(legit_ann.origin,
                                                      legit_ann.prefix.addr,
                                                      legit_ann.prefix.netmask,
                                                      4,
                                                      asn,
                                                      false,
                                                      std::set<Prefix<>>(),
                                                      hijack_ann);
  neg_ann.null_route_subprefix(hijack_ann.prefix);
  // Add to set of negative_announcements
  negative_announcements.insert(neg_ann);
  // Add to Announcements to propagate
  incoming_announcements->push_back(neg_ann);
  // Add Announcement to blocked list (i.e. blackhole list)
  blackhole_map[hijack_ann.prefix] = hijack_ann;
}


std::pair<bool, Announcement*> ROVppAS::received_valid_announcement(Announcement &announcement) {
  // Check if it's in all_anns (i.e. RIB in)
  for (auto& ann : *all_anns) {
    if (ann.first < announcement.prefix && ann.second.origin == victim_asn) {
      return std::make_pair(true, &ann.second);
    }
  }

  // Check if it was just received and hasn't been processed yet
  for (Announcement& ann : *incoming_announcements) {
    if (ann.prefix < announcement.prefix && ann.origin == victim_asn) {
      return std::make_pair(true, &ann);
    }
  }

  // Otherwise
  return std::make_pair(false, nullptr);
}

std::pair<bool, Announcement*> ROVppAS::received_hijack_announcement(Announcement &announcement) {
  // Check if announcement is in dropped list
  for (std::pair<const Prefix<>, Announcement>& prefix_ann_pair : dropped_ann_map) {
    if (announcement.prefix < prefix_ann_pair.first && prefix_ann_pair.second.origin == attacker_asn) {
      return std::make_pair(true, &prefix_ann_pair.second);
    }
  }

  // Check if the announcement is in the blackhole list
  for (std::pair<const Prefix<>, Announcement>& prefix_ann_pair : blackhole_map) {
    if (announcement.prefix < prefix_ann_pair.first && prefix_ann_pair.second.origin == attacker_asn) {
      return std::make_pair(true, &prefix_ann_pair.second);
    }
  }

  // otherwise
  return std::make_pair(false, nullptr);
}


bool ROVppAS::paths_intersect(Announcement &legit_ann, Announcement &hijack_ann) {
  // Get the paths for the announcements
  // Announcement legit_ann_copy = legit_ann;
  // Announcement hijack_ann_copy = hijack_ann;
  std::vector<uint32_t> legit_path = get_as_path(legit_ann);
  std::vector<uint32_t> hijacked_path = get_as_path(hijack_ann);
  // Double for loop to check over entire (except the beginning and end of each path)
  for (std::size_t i=1; i<legit_path.size(); ++i) {
    for (std::size_t j=1; j<hijacked_path.size(); ++j) {
      if (legit_path[i] == hijacked_path[j]) {
        return true;
      }
    }
  }
  // No hops of the paths match
  return false;
}

std::vector<uint32_t> ROVppAS::get_as_path(Announcement &ann) {
  // will hold the entire path from origin to this node,
  // including itself
  std::vector<uint32_t> as_path;
  uint32_t current_asn = asn;  // it's own ASN
  AS * curr_as = this;
  uint32_t origin_asn = ann.origin;  // the origin AS of the announcement
  Announcement * curr_announcement = &ann;
  // append to end of ASN path
  as_path.push_back(current_asn);
  // Add to the path vector until we reach the origin
  while (current_asn != origin_asn) {
    // Check if the received from asn is 0
    // This means it's coming from an AS which was seeded with this announcement
    // Just take the origin as the ASN in this case
    if (curr_announcement->received_from_asn == 0) {
      as_path.push_back(curr_announcement->origin);
      break;
    }
    // Get the previous AS in path
    current_asn = curr_announcement->received_from_asn;
    curr_as = as_graph->get_as_with_asn(current_asn);
    // Get same announcement from previous AS
    curr_announcement = curr_as->get_ann_for_prefix(curr_announcement->prefix);
    // append to end of ASN path
    as_path.push_back(current_asn);
  }
  return as_path;
}


std::ostream& ROVppAS::stream_blacklist(std:: ostream &os) {
  for (std::pair<Prefix<>, Announcement> prefix_ann_pair : blackhole_map){
      // rovpp_asn,prefix,hijacked_ann_received_from_asn
      os << asn << ",";
      prefix_ann_pair.second.to_csv(os);
  }
  return os;
}
