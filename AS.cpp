#include "AS.h"

/** Constructor for AS class.
 *
 * AS objects represent a node in the AS Graph.
 */
AS::AS(uint32_t myasn, 
    std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inv, 
    std::set<uint32_t> *prov, std::set<uint32_t> *peer,
    std::set<uint32_t> *cust) {
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

AS::~AS() {
    delete incoming_announcements;
    delete anns_sent_to_peers_providers;
    delete all_anns;
    delete peers;
    delete providers;
    delete customers;
    delete member_ases;
}


/** Add neighbor AS to the appropriate set in this AS based on the relationship.
 *
 * @param asn ASN of neighbor.
 * @param relationship AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
 */
void AS::add_neighbor(uint32_t asn, int relationship) {
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


/** Debug to print this AS number
 */
void AS::printDebug() {
    std::cout << asn << std::endl;
}


/** Push the received announcements to the incoming_announcements vector. 
 *
 * Note that this differs from the Python version in that it does not store
 * a dict of (prefix -> list of announcements for that prefix).
 *
 * @param announcements to be pushed onto the incoming_announcements vector.
 */
void AS::receive_announcements(std::vector<Announcement> &announcements) {
    for (Announcement &ann : announcements) {
        // Do not check for duplicates here
        // push_back makes a copy of the announcement
        incoming_announcements->push_back(ann);
    }
}


/** 
 *
 * may need to put in incoming_announcements for speed
 * called by ASGraph.give_ann_to_as_path()
 */ 
void AS::receive_announcement(Announcement &ann) {
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
                depref_anns->insert(std::pair<Prefix<>, Announcement>(search->second.prefix, 
                                                                      search->second));
                search->second = ann;
            } else {
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
            // Insert new second best announcement
            depref_anns->insert(std::pair<Prefix<>, Announcement>(search->second.prefix, 
                                                                  search->second));
            // Replace the old announcement with the higher priority
            search->second = ann;
        } else {
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


/** Iterate through incoming_announcements and keep only the best. 
 * 
 * Meant to approximate BGP best path selection.
 */
void AS::process_announcements() {
    for (auto &ann : *incoming_announcements) {
        auto search = all_anns->find(ann.prefix);
        if (search == all_anns->end() || !search->second.from_monitor) {
            receive_announcement(ann);
        }
    }
    incoming_announcements->clear();
}


/** Clear all announcement collections. 
 */
void AS::clear_announcements() {
    all_anns->clear();
    incoming_announcements->clear();
    anns_sent_to_peers_providers->clear();
}


/** Check if annoucement is already recv'd by this AS. 
 *
 * @param ann Announcement to check for. 
 * @return True if recv'd, false otherwise.
 */
bool AS::already_received(Announcement &ann) {
    auto search = all_anns->find(ann.prefix);
    return (search == all_anns->end()) ? false : true;
}


/** Insertion operator for AS class.
 *
 * @param os
 * @param as
 * @return os passed as parameter
 */
std::ostream& operator<<(std::ostream &os, const AS& as) {
    os << "ASN: " << as.asn << std::endl << "Rank: " << as.rank
        << std::endl << "Providers: ";
    for (auto &provider : *as.providers) {
        os << provider << " ";
    }
    os << std::endl;
    os << "Peers: ";
    for (auto &peer : *as.peers) {
        os << peer << " ";
    }
    os << std::endl;
    os << "Customers: ";
    for (auto &customer : *as.customers) {
        os << customer << " ";
    }
    os << std::endl;
    return os;
}


/** Streams announcements to an output stream in a .csv readable file format.
 *
 * @param os
 * @return output stream into which is passed the .csv row formatted announcements
 */
std::ostream& AS::stream_announcements(std::ostream &os){
    for (auto &ann : *all_anns){
        os << asn << ",";
        ann.second.to_csv(os);
    }
    return os;
}

