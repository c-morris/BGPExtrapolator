#include <cmath>
#include "Extrapolator.h"

Extrapolator::Extrapolator() {
    ases_with_anns = new std::vector<uint32_t>;
}

/** Propagate announcements from customers to peers and providers.
 */
void Extrapolator::propagate_up() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = 0; level < levels; level++) {
        for (uint32_t asn : graph->ases_by_rank[level]) {
            graph->ases->find(asn)->second->process_announcements();
            // if (graph.ases[asn].all_anns):
            //     self.send_all_announcements(asn, to_peers_providers = True,
            //     to_customers = False)
        }
    }
}

/** Send "best" announces to customer ASes. 
 */
void Extrapolator::propagate_down() {
    size_t levels = graph->ases_by_rank->size();
    for (size_t level = levels-1; level >= 0; level--) {
        for (uint32_t asn : graph->ases_by_rank[level]) {
            graph->ases->find(asn)->second->process_announcements();
            // if (graph.ases[asn].all_anns):
            //     self.send_all_announcements(asn, to_peers_providers = True,
            //     to_customers = False)
        }
    }
}

/** Send all announcements kept by an AS to its neighbors. 
 *
 * This approximates the Adj-RIBs-out. 
 *
 * @param asn AS that is sending out announces
 * @param to_peers_providers send to peers and providers
 * @param to_customers send to customers
 */
void Extrapolator::send_all_announcements(uint32_t asn, 
    bool to_peers_providers = false, 
    bool to_customers = false) {
    auto *source_as = graph->ases->find(asn)->second; 
    if (to_peers_providers) {
        std::vector<Announcement> anns_to_providers;
        std::vector<Announcement> anns_to_peers;
        for (auto &ann : *source_as->all_anns) {
            if (ann.second.priority < 2) 
                continue;
            // priority is reduced by 0.01 for path length
            // base priority is 2 for providers
            // base priority is 1 for peers
            double priority = ann.second.priority;
                priority = priority - floor(priority) - 0.01 + 2;
            anns_to_providers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn));

            priority--; // subtract 1 for peers
            anns_to_peers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn));
        }
        // send announcements
        for (uint32_t provider_asn : *source_as->providers) {
            graph->ases->find(provider_asn)->second->receive_announcements(
                anns_to_providers);
        }
        for (uint32_t peer_asn : *source_as->peers) {
            graph->ases->find(peer_asn)->second->receive_announcements(
                anns_to_peers);
        }
    }

    // now do the same for customers
    if (to_customers) {
        std::vector<Announcement> anns_to_customers;
        for (auto &ann : *source_as->all_anns) {
            if (ann.second.priority < 2) 
                continue;
            // priority is reduced by 0.01 for path length
            // base priority is 0 for customers
            double priority = ann.second.priority;
                priority = priority - floor(priority) - 0.01;
            anns_to_customers.push_back(
                Announcement(
                    ann.second.origin,
                    ann.second.prefix.addr,
                    ann.second.prefix.netmask,
                    priority,
                    asn));
        }
        // send announcements
        for (uint32_t customer_asn : *source_as->customers) {
            graph->ases->find(customer_asn)->second->receive_announcements(
                anns_to_customers);
        }
    }
}
