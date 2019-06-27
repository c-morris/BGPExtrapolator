#include <cstdint>
#include <math.h>
#include "ROVppAS.h"
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
    // Check if it's from the same neighbor
    if (ann.received_from_asn == check.second->received_from_asn) {
      // Okay, before we admit we're screwed, see if we have an alternative route
      std::pair<bool, Announcement> alternate_route = best_alternative_route(ann);
      if (alternate_route.first) {
        (*all_anns)[alternate_route.second.prefix] = alternate_route.second;
        if (!alternate_route.second.has_blackholes) {
          // Remove Blackhole
          std::cout << "Removing Blackhole for " << ann.get_blackhole_prefix().to_cidr() << " because we have a hole free alternative." << std::endl;
          blackhole_map.erase(ann.get_blackhole_prefix());
        }
      } else {
        make_blackhole_announcement(ann);
      }
    }
  } else {
    make_blackhole_announcement(ann);
  }
}


void ROVppAS::incoming_valid_announcement(Announcement &ann) {
  // See if we already have announcement for this prefix
  // makes a copy of the announcement
  auto search = all_anns->find(ann.prefix);
  if (search != all_anns->end()) {
    if (is_better(ann, search->second)) {
        (*all_anns)[ann.prefix] = ann;
        // dropped_ann_map.erase(ann.prefix);
        // blackhole_map.erase(ann.prefix);
    }
  } else {
    (*all_anns)[ann.prefix] = ann;
  }
  // Check if you should make blackhole announcement (now that you have the positive part)
  std::pair<bool, Announcement*> check = received_hijack_announcement(ann);
  if (check.first) {
    // Check if it's coming from the same ASN as the current one you're using
    if ((*all_anns)[ann.prefix].received_from_asn == (*(check.second)).received_from_asn) {
      make_negative_announcement_and_blackhole((*all_anns)[ann.prefix], *(check.second), true);
    } else {
      // Remove Blackhole
      std::cout << "Removing Blackhole for " << (*(check.second)).prefix.to_cidr() << " because we have a hole free alternative." << std::endl;
      blackhole_map.erase((*(check.second)).prefix);
    }
  }
}


void ROVppAS::incoming_hijack_announcement(Announcement &ann) {
  // Drop the announcement (i.e. don't add it to incoming_announcements)
  // Add announcement to dropped list
  std::cout << "Dropping announcement for prefix "<< ann.prefix.to_cidr() << " from AS " << ann.received_from_asn << std::endl;
  dropped_ann_map[ann.prefix] = ann;
  // Check if should make negative announcement
  // Check if you have received the Valid announcement in the past
  // This check needs to be made in order to know if we have a path to the legit origin (otherwise, we may not have such a path yet).
  // Check if you have an alternative route (i.e. escape route)
  std::pair<bool, Announcement*> check = received_valid_announcement(ann);
  if (check.first) {
    // Check if it's from the same neighbor
    if (ann.received_from_asn == check.second->received_from_asn) {
      // Okay, before we admit we're screwed, see if we have an alternative route
      std::pair<bool, Announcement> alternate_route = best_alternative_route(ann);
      if (alternate_route.first) {
        (*all_anns)[alternate_route.second.prefix] = alternate_route.second;
        if (!alternate_route.second.has_blackholes) {
          // Remove Blackhole
          std::cout << "Removing Blackhole for " << ann.prefix.to_cidr() << " because we have a hole free alternative." << std::endl;
          blackhole_map.erase(ann.prefix);
        }
      } else {
        make_negative_announcement_and_blackhole(*(check.second), ann, true);
      }
    }
  }
}

/** Processes hazards from freinds
*/
void ROVppAS::incoming_hazard_announcement(Announcement ann, uint32_t publishing_asn) {
  // Check if it's in all_anns (i.e. RIB in)
  if (!all_anns->empty()) {
    for (auto& prefix_ann_pair : *all_anns) {
      if (paths_intersect(prefix_ann_pair.second, ann)) {
        // Okay, before we admit we're screwed, see if we have an alternative route
        std::pair<bool, Announcement> alternate_route = best_alternative_route(prefix_ann_pair.second);
        if (alternate_route.first) {
          (*all_anns)[alternate_route.second.prefix] = alternate_route.second;
          if (!alternate_route.second.has_blackholes) {
            // Remove Blackhole
            std::cout << "Removing Blackhole for " << ann.prefix.to_cidr() << " because we have a hole free alternative." << std::endl;
            blackhole_map.erase(ann.prefix);
          }
        } else {
          // Blackhole the prefix
          make_negative_announcement_and_blackhole(prefix_ann_pair.second, ann, false);
        }
      }
    }
  } else {
    // Make Blackhole for hijacked prefix
    std::cout << "Adding to Blackhole list Hazard for prefix "<< ann.prefix.to_cidr() << " from Publishing AS " << publishing_asn << std::endl;
    // Add Announcement to blocked list (i.e. blackhole list)
    blackhole_map[ann.prefix] = ann;
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
    if (pass_rov(ann)) {
      // Add ann to considerable announcements history
      ann_history[ann.prefix].insert(ann);
    }
    incoming_announcements->push_back(ann);
  }
}


bool ROVppAS::is_better(Announcement &new_ann, Announcement &curr_ann) {
  // Check if BGP Relationship is greater
  // Customers = 3, Peers = 2, Providers = 1
  double new_ann_rel = std::ceil(new_ann.priority);
  double curr_ann_rel = std::ceil(curr_ann.priority);

  // Precompute some conditions
  // Check if relationship is the same
  bool same_relationship = new_ann_rel == curr_ann_rel;
  // Check blackhole size
  bool same_blackhole_size = new_ann.blackholed_prefixes.size() == curr_ann.blackholed_prefixes.size();

  // Check if BGP Relationship is greater
  if (new_ann_rel > curr_ann_rel) {
    return true;
    // Check blackhole size
  } else if (same_relationship && new_ann.blackholed_prefixes.size() < curr_ann.blackholed_prefixes.size()) {
    return true;
    // Use BGP priority to make decision
  } else if (same_blackhole_size && new_ann.priority > curr_ann.priority) {
    return true;
  } else {
    return false;
  }
}


/**
 *
 * may need to put in incoming_announcements for speed
 * called by ASGraph.give_ann_to_as_path()
 */
void ROVppAS::receive_announcement(Announcement &ann) {
  // Check for existing annoucement for prefix
  // auto search = all_anns->find(ann.prefix);
  // auto search_alt = depref_anns->find(ann.prefix);
  // auto search_history = ann_history[ann.prefix].find(ann.prefix);

  // Check if it's a NegativeAnnouncement
  if (ann.has_blackholes) {
    incoming_negative_announcement(ann);
  } else if (pass_rov(ann)) {  // It's not a negative announcement. Check if the Announcement is valid
    incoming_valid_announcement(ann);
  } else {  // Not valid (i.e. hijack announcement)
    incoming_hijack_announcement(ann);
  }

  // // No announcement found for incoming announcement prefix
  // if (search == all_anns->end()) {
  //     all_anns->insert(std::pair<Prefix<>, Announcement>(ann.prefix, ann));
  //
  //   // See if this announcement is better than the one we're currently using
  // } else if (is_better(ann, *(search->second))) {
  //     // Replace the one we're currenlty using with the better one
  //     search->second = ann;
  // }
}


/** Make negative announcement AND Blackhole's that announcement
*/
void ROVppAS::make_negative_announcement_and_blackhole(Announcement &legit_ann, Announcement &hijack_ann, bool publish) {
  // Create Negative Announcement
  std::cout << "Making Blackhole at AS" << asn << " for prefix: " << hijack_ann.prefix.to_cidr() << std::endl;
  Announcement neg_ann = Announcement(legit_ann.origin,
                                      legit_ann.prefix.addr,
                                      legit_ann.prefix.netmask,
                                      legit_ann.priority,
                                      legit_ann.received_from_asn,
                                      false);
  neg_ann.add_blackhole(hijack_ann.prefix);
  // Add to set of negative_announcements
  negative_announcements.insert(neg_ann);
  // Add to Announcements to propagate
  (*all_anns)[neg_ann.prefix] = neg_ann;
  // Add Announcement to blocked list (i.e. blackhole list)
  blackhole_map[hijack_ann.prefix] = hijack_ann;

  // Publish Hazard
  if (friends_enabled && publish) {
    as_graph->publish_harzard(hijack_ann, asn);
  }
}


void ROVppAS::make_blackhole_announcement(Announcement &neg_ann) {
  // Create Negative Announcement
  Prefix<> blackhole_prefix = neg_ann.get_blackhole_prefix();  // TODO: This needs to be changed if we're doing more than one hijack per simulation
  std::cout << "Making Blackhole at AS" << asn << " for prefix: " << blackhole_prefix.to_cidr() << std::endl;
  Announcement new_neg_ann = Announcement(neg_ann.origin,
                                          neg_ann.prefix.addr,
                                          neg_ann.prefix.netmask,
                                          neg_ann.priority,
                                          neg_ann.received_from_asn,
                                          false);
  new_neg_ann.add_blackhole_set(neg_ann.blackholed_prefixes);
  // Add to set of negative_announcements
  negative_announcements.insert(new_neg_ann);
  // Add to Announcements to propagate
  (*all_anns)[new_neg_ann.prefix] = new_neg_ann;
  // Add Announcement to blocked list (i.e. blackhole list)
  blackhole_map[blackhole_prefix] = new_neg_ann;
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
    if (curr_announcement->priority > 1) {
      break;
    }
    // append to end of ASN path
    as_path.push_back(current_asn);
  }
  return as_path;
}

/** This method is called when you get a blackhole annoucement, and you want to
* know if you have an alternative path besides this one you currently using.
*/
std::pair<bool, Announcement> ROVppAS::best_alternative_route(Announcement &blackhole_ann) {
  // Check if the prefix is in our history
  auto search = ann_history.find(blackhole_ann.prefix);
  if (search != ann_history.end()) {
    // Get the set of annnoucements for this prefix
    std::set<Announcement> announcements_for_prefix = search->second;
    // If Prefering ROV++ nodes are enabled
    if (preference_enabled) {
      // Check if we have an ROV++ neighbor alternative
      for (Announcement ann_candidate : announcements_for_prefix) {
        if (as_graph->implements_rovpp(ann_candidate.received_from_asn)) {
          if (is_better(ann_candidate, blackhole_ann)) {
            return std::make_pair(true, ann_candidate);
          }
        }
      }
    }
    // Otherwise, we'll settle for a non-ROV++ neighbor
    for (Announcement ann_candidate : announcements_for_prefix) {
      if (is_better(ann_candidate, blackhole_ann)) {
        return std::make_pair(true, ann_candidate);
      }
    }
  }
  return std::make_pair(false, Announcement());
}


std::ostream& ROVppAS::stream_blacklist(std:: ostream &os) {
  for (std::pair<Prefix<>, Announcement> prefix_ann_pair : blackhole_map){
      // rovpp_asn,prefix,hijacked_ann_received_from_asn
      os << asn << ",";
      prefix_ann_pair.second.to_csv(os);
  }
  return os;
}
