#include "AS.h"

AS::AS(uint32_t myasn, std::set<uint32_t> *prov, std::set<uint32_t> *peer,
    std::set<uint32_t> *cust) {
    asn = myasn;
    rank = -1; // initialize to invalid rank
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
    anns_sent_to_peers_providers = new std::vector<Announcement>;
    incoming_announcements = new std::vector<Announcement>;
    all_anns = new std::map<Prefix, Announcement>;
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
}

/** Add neighbor AS to the appropriate set based on the relationship.
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

/** Update rank of this AS only if newrank is greater than the current rank.
 *
 * @param newrank The height of the AS in the provider->customer DAG. Used for simple propagation up and down the graph. 
 * @return True if rank was changed, false otherwise. 
 */
bool AS::update_rank(int newrank) {
    int old = rank;
    rank = std::max(rank, newrank);
    return (old != rank) ? true : false;
}

// print asn only
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
        // do not check for duplicates here-- hopefully this is fine
        // push_back should make a copy of the announcement
        incoming_announcements->push_back(ann);
    }
}

void AS::receive_announcement(Announcement &ann) {
    incoming_announcements->push_back(ann);
}

/** Iterate through incoming_announcements and keep only the best. 
 * Meant to approximate BGP best path selection.
 */
void AS::process_announcements() {
    // I skipped a few steps here that seemed unnecessary. 
    // If it turns out that they are necessary, I'm sorry.
    // Let me know and I'll fix it. 
    for (const auto &ann : *incoming_announcements) {
        // from https://en.cppreference.com/w/cpp/container/map/find
        auto search = all_anns->find(ann.prefix);
        if (search == all_anns->end()) {
            all_anns->insert(std::pair<Prefix, Announcement>(
                ann.prefix, ann));
        } else if (ann.priority > search->second.priority) {
            search->second = ann;
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

