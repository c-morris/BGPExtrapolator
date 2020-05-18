#ifndef ROVPP_EXTRAPOLATOR_H
#define ROVPP_EXTRAPOLATOR_H

#include "Extrapolators/BaseExtrapolator.h"
#include "Graphs/ROVppASGraph.h"
#include "Announcements/ROVppAnnouncement.h"

struct ROVppExtrapolator: public BaseExtrapolator<ROVppSQLQuerier, ROVppASGraph, ROVppAnnouncement, ROVppAS> {
    ROVppExtrapolator(std::vector<std::string> g=std::vector<std::string>(),
                      bool random_b=true,
                      std::string r=RESULTS_TABLE,
                      std::string e=VICTIM_TABLE,
                      std::string f=ATTACKER_TABLE,
                      uint32_t iteration_size=false);
    ~ROVppExtrapolator();

    void perform_propagation(bool propogate_twice);

    void process_withdrawal(uint32_t asn, ROVppAnnouncement ann, ROVppAS *neighbor);
    void process_withdrawals(ROVppAS *as);
    bool loop_check(Prefix<> p, const ROVppAS& cur_as, uint32_t a, int d); 

    uint32_t get_priority(ROVppAnnouncement const& ann, uint32_t i);
    bool is_filtered(ROVppAS *rovpp_as, ROVppAnnouncement const& ann);

    ////////////////////////////////////////////////////////////////////
    // Overidden Methods
    ////////////////////////////////////////////////////////////////////
    
    void give_ann_to_as_path(std::vector<uint32_t>*, 
                             Prefix<> prefix, 
                             int64_t timestamp, 
                             bool hijack);
    void propagate_up();
    void propagate_down();
    void send_all_announcements(uint32_t asn,
                                bool to_providers = false,
                                bool to_peers = false,
                                bool to_customers = false);
    void save_results(int iteration);
};

#endif