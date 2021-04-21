#ifndef BLOCKED_EXTRAPOLATOR_H
#define BLOCKED_EXTRAPOLATOR_H

#define DEFAULT_ITERATION_SIZE 50000
#define DEFAULT_MH_MODE 1

#include "Extrapolators/BaseExtrapolator.h"

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType, typename PrefixType = uint32_t>
class BlockedExtrapolator : public BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>  {
protected:
    uint32_t iteration_size;
    uint32_t mh_mode;

    /**
     *  Overrwritable function that is first called in the preform_propagation function.
     *  Purely here for inheritance reasons 
     */
    virtual void init();

    /**
     *  Overrwritable function that is called after populate_blocks in the preform_propagation function.
     *  Purely here for inheritance reasons.
     */
    virtual void extrapolate(std::vector<Prefix<PrefixType>*> *prefix_blocks, std::vector<Prefix<PrefixType>*> *subnet_blocks);

public:
    BlockedExtrapolator(bool random_tiebraking,
                        bool store_results, 
                        bool store_invert_results, 
                        bool store_depref_results,
                        uint32_t iteration_size,
                        uint32_t mh_mode,
                        bool origin_only,
                        std::vector<uint32_t> *full_path_asns,
                        int max_threads) : BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>(random_tiebraking, store_results, store_invert_results, store_depref_results, origin_only, full_path_asns, max_threads) {
        
        this->iteration_size = iteration_size;
        this->mh_mode = mh_mode;
    }

    BlockedExtrapolator() : BlockedExtrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_RESULTS, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, DEFAULT_ITERATION_SIZE, DEFAULT_MH_MODE, DEFAULT_ORIGIN_ONLY, NULL, DEFAULT_MAX_THREADS) { }

    virtual ~BlockedExtrapolator();

    /** Performs all tasks necessary to propagate a set of announcements given:
     *      1) A populated mrt_announcements table
     *      2) A populated customer_provider table
     *      3) A populated peers table
     */
    virtual void perform_propagation();

    /** Recursive function to break the input mrt_announcements into manageable blocks.
     *
     * @param p The current subnet for checking announcement block size
     * @param prefix_vector The vector of prefixes of appropriate size
     */
    virtual void populate_blocks(Prefix<PrefixType>*, 
                                    std::vector<Prefix<PrefixType>*>*, 
                                    std::vector<Prefix<PrefixType>*>*);

    /** Process a set of prefix or subnet blocks in iterations.
    */
    virtual void extrapolate_blocks(uint32_t &announcement_count, 
                                    int &iteration, 
                                    bool subnet, 
                                    std::vector<Prefix<PrefixType>*> *prefix_set);

    /** Seed announcement on all ASes on as_path. 
     *
     * The from_monitor attribute is set to true on these announcements so they are
     * not replaced later during propagation. 
     * 
     * Must be implemented by child class!
     *
     * @param as_path Vector of ASNs for this announcement.
     * @param prefix The prefix this announcement is for.
     */
    virtual void give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<PrefixType> prefix, int64_t timestamp = 0);

    /** Send all announcements kept by an AS to its neighbors. 
     *
     * This approximates the Adj-RIBs-out. 
     *
     * @param asn AS that is sending out announces
     * @param to_providers Send to providers
     * @param to_peers Send to peers
     * @param to_customers Send to customers
     */
    virtual void send_all_announcements(uint32_t asn, bool to_providers = false, bool to_peers = false, bool to_customers = false);
};

#endif
