#ifndef ROV_EXTRAPOLATOR_H
#define ROV_EXTRAPOLATOR_H

#include "Extrapolators/BlockedExtrapolator.h"

#include "SQLQueriers/ROVSQLQuerier.h"
#include "Graphs/ROVASGraph.h"
#include "Announcements/ROVAnnouncement.h"
#include "ASes/ROVAS.h"

/**
 * This Extrapolator implements ROV policy
 * In short, ASes that adopt ROV check whether an announcement is valid
 * by looking at its origin. If the origin AS is an attacker, the announcement is rejected
*/
class ROVExtrapolator : public BlockedExtrapolator<ROVSQLQuerier, ROVASGraph, ROVAnnouncement, ROVAS> {
public:
    ROVExtrapolator(bool random_tiebraking,
                    bool store_results, 
                    std::vector<std::string> policy_tables,
                    std::string announcement_table,
                    std::string results_table, 
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

    /** Process a set of prefix or subnet blocks in iterations.
    */
    void extrapolate_blocks(uint32_t &announcement_count, int &iteration, bool subnet, std::vector<Prefix<>*> *prefix_set);

    /** Seed announcement on all ASes on as_path. 
     *
     * The from_monitor attribute is set to true on these announcements so they are
     * not replaced later during propagation. 
     * 
     * @param as_path Vector of ASNs for this announcement.
     * @param prefix The prefix this announcement is for.
     * @param timestamp Timestamp of this announcement.
     * @param roa_validity Validity of this announcement. 0 = valid, 1 = unknown, etc.
     */
    void give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp = 0, uint32_t roa_validity = 1);
};

#endif
