#ifndef EXTRAPOLATOR_H
#define EXTRAPOLATOR_H

#include <vector>

#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Prefix.h"

class Extrapolator {
    // database querier here?
    ASGraph *graph;
    std::vector<uint32_t> *ases_with_anns;

    Extrapolator();
    void send_all_announcements(uint32_t asn, 
        bool to_peers_providers, 
        bool to_customers);
    void insert_announcements(std::vector<Prefix> *prefixes);
    void prop_anns_sent_to_peers_providers();
    void propagate_up();
    void propagate_down();
    void give_ann_to_as_path(std::vector<uint32_t>* as_path, 
        Prefix prefix,
        uint32_t hop);
};

#endif
