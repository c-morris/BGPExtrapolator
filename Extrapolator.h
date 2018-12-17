#ifndef EXTRAPOLATOR_H
#define EXTRAPOLATOR_H

#include <vector>
#include <bits/stdc++.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Prefix.h"
#include "SQLQuerier.h"


struct Extrapolator {
    ASGraph *graph;
    SQLQuerier *querier;
    Extrapolator();
    ~Extrapolator();
    std::set<uint32_t> *ases_with_anns;

    void perform_propagation(bool test = false, int group_size = 1000, int max_total = 0);
    void send_all_announcements(uint32_t asn, 
        bool to_peers_providers = false, 
        bool to_customers = false);
    void insert_announcements(std::vector<Prefix> *prefixes);
    void prop_anns_sent_to_peers_providers();
    void propagate_up();
    void propagate_down();
    void give_ann_to_as_path(std::vector<uint32_t>* as_path, 
        Prefix prefix,
        uint32_t hop);
    };

#endif
