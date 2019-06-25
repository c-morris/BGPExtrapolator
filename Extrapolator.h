#ifndef EXTRAPOLATOR_H
#define EXTRAPOLATOR_H

#include <vector>
#include <bits/stdc++.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <dirent.h>

#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Prefix.h"
#include "SQLQuerier.h"
#include "TableNames.h"

struct Extrapolator {
    ASGraph *graph;
    SQLQuerier *querier;
    Extrapolator(bool invert_results=true, std::string
        a=ANNOUNCEMENTS_TABLE, std::string r=RESULTS_TABLE, std::string
        i=INVERSE_RESULTS_TABLE, bool ram_tablespace=false);
    Extrapolator(
        std::uint32_t attacker_asn, std::uint32_t victim_asn, std::string victim_prefix,
        bool enable_negative_anns, bool enable_friends, bool enable_preferences,
        bool invert_results=true, std::string
        a=ANNOUNCEMENTS_TABLE, std::string r=RESULTS_TABLE, std::string
        i=INVERSE_RESULTS_TABLE, bool ram_tablespace=false);
    ~Extrapolator();
    std::set<uint32_t> *ases_with_anns;
    std::vector<std::thread> *threads;
    bool invert;
    bool ram_tablespace;

    void perform_propagation(bool test = false, size_t group_size = 1000, size_t max_total = 0);
    void send_all_announcements(uint32_t asn,
        bool to_providers = false,
        bool to_peers = false,
        bool to_customers = false);
    void insert_announcements(std::vector<Prefix<>> *prefixes);
    void prop_anns_sent_to_peers_providers();
    std::vector<uint32_t>* parse_path(std::string path_as_string);
    void propagate_up();
    void propagate_down();
    void give_ann_to_as_path(std::vector<uint32_t>* as_path,
        Prefix<> prefix);
    void save_results(int iteration);
    void invert_results(void);
    void print_results(int iter);
    };

#endif
