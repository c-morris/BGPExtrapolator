#include <cstdint>
#include "ROVAS.h"
#include "NegativeAnnouncement.h"


ROVAS::ROVAS(uint32_t myasn,
    std::uint32_t attacker_asn,
    std::uint32_t victim_asn,
    std::string victim_prefix,
    std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inv,
    ASGraph *graph,
    std::set<uint32_t> *prov,
    std::set<uint32_t> *peer,
    std::set<uint32_t> *cust) {

  // Simulation variables
  this->attacker_asn = attacker_asn;
  this->victim_asn = victim_asn;
  this->victim_prefix = victim_prefix;

  // Save graph reference
  as_graph = graph;

  asn = myasn;
  // Initialize AS to invalid rank
  rank = -1;

  // Generate AS relationship sets
  if (prov == NULL) {
      providers = new std::set<uint32_t>;
  } else {
      providers = prov;
  }
  if (peer == NULL) {
      peers = new std::set<uint32_t>;
  } else {
      peers = peer;
  }
  if (cust == NULL) {
      customers = new std::set<uint32_t>;
  } else {
      customers = cust;
  }

  inverse_results = inv;
  member_ases = new std::vector<uint32_t>;
  anns_sent_to_peers_providers = new std::vector<Announcement>;
  incoming_announcements = new std::vector<Announcement>;
  all_anns = new std::map<Prefix<>, Announcement>;
  depref_anns = new std::map<Prefix<>, Announcement>;
  index = -1;
  onStack = false;
}

ROVAS::~ROVAS() {
    delete incoming_announcements;
    delete anns_sent_to_peers_providers;
    delete all_anns;
    delete peers;
    delete providers;
    delete customers;
    delete member_ases;
    delete as_graph;
}


bool ROVAS::pass_rov(Announcement &ann) {
  if (ann.origin == attacker_asn) {
    return false;
  } else {
    return true;
  }
}

/** Push the received announcements to the incoming_announcements vector.
 *
 * Note that this differs from the Python version in that it does not store
 * a dict of (prefix -> list of announcements for that prefix).
 *
 * @param announcements to be pushed onto the incoming_announcements vector.
 */
void ROVAS::receive_announcements(std::vector<Announcement> &announcements) {
  for (Announcement &ann : announcements) {
    // Check if it's a negative annoucement
    if (NegativeAnnouncement* d = dynamic_cast<NegativeAnnouncement*>(&ann)) {
      // Drop the announcement
    } else {
      // Check if the Announcement is valid
      if (pass_rov(ann)) {
        // Do not check for duplicates here
        // push_back makes a copy of the announcement
        incoming_announcements->push_back(ann);
      } else {
        blocked_map[ann.prefix] = ann;
      }
    }
  }
}
