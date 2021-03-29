#ifndef ROV_EXTRAPOLATOR_H
#define ROV_EXTRAPOLATOR_H

#include "Extrapolators/BlockedExtrapolator.h"

#include "SQLQueriers/ROVSQLQuerier.h"
#include "Graphs/ROVASGraph.h"
#include "Announcements/ROVAnnouncement.h"
#include "ASes/ROVAS.h"

class ROVExtrapolator : public BlockedExtrapolator<ROVSQLQuerier, ROVASGraph, ROVAnnouncement, ROVAS> {
public:
    ROVExtrapolator(bool random_tiebraking,
                    bool store_results, 
                    bool store_invert_results, 
                    bool store_depref_results, 
                    std::vector<std::string> policy_tables,
                    std::string announcement_table,
                    std::string results_table, 
                    std::string inverse_results_table, 
                    std::string depref_results_table, 
                    std::string full_path_results_table, 
                    std::string config_section,
                    uint32_t iteration_size,
                    int exclude_as_number,
                    uint32_t mh_mode,
                    bool origin_only,
                    std::vector<uint32_t> *full_path_asns,
                    int max_threads);
    
    ROVExtrapolator();
    ~ROVExtrapolator();

    void extrapolate_blocks(uint32_t &announcement_count, int &iteration, bool subnet, std::vector<Prefix<>*> *prefix_set);
    void give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp = 0, uint32_t roa_validity = 1);
};

#endif
