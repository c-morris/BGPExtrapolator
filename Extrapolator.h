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
    bool invert;
    bool depref;
    bool origin_o;
    bool mrt_o;
    uint32_t it_size;
    uint32_t vf_as;
    ASGraph *graph;
    SQLQuerier *querier;
    std::vector<std::thread> *threads;

    Extrapolator(bool invert_results=true, 
                 bool store_depref=false, 
                 bool origin_only=false,
                 bool mrt_only=false,
                 std::string a=ANNOUNCEMENTS_TABLE,
                 std::string r=RESULTS_TABLE, 
                 std::string i=INVERSE_RESULTS_TABLE, 
                 std::string d=DEPREF_RESULTS_TABLE, 
                 std::string v=VERIFICATION_TABLE, 
                 uint32_t iteration_size=false,
                 uint32_t verification_as=false);
    ~Extrapolator();
    
    void perform_propagation(bool, size_t);
    template <typename Integer>
    void populate_blocks(Prefix<Integer>*, 
                         std::vector<Prefix<>*>*, 
                         std::vector<Prefix<>*>*);
    
    void store_vf_ann(std::string,
                      uint32_t,
                      std::string);

    void send_all_announcements(uint32_t asn, 
                                bool to_providers = false, 
                                bool to_peers = false, 
                                bool to_customers = false);
    
    void insert_announcements(std::vector<Prefix<>> *prefixes);
    void prop_anns_sent_to_peers_providers();
    std::vector<uint32_t>* parse_path(std::string path_as_string);
    void propagate_up();
    void propagate_down();
    void give_origin_to_as_path(std::vector<uint32_t>* as_path, 
                             Prefix<> prefix);
    void give_ann_to_as_path(std::vector<uint32_t>* as_path, 
                             Prefix<> prefix);
    void save_results(int iteration);
    void invert_results(void);
    };

#endif
