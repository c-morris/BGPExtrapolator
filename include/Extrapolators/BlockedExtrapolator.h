#ifndef BLOCKED_EXTRAPOLATOR_H
#define BLOCKED_EXTRAPOLATOR_H

#include "Extrapolators/BaseExtrapolator.h"

template <class SQLQuerierType, class GraphType, class AnnouncementType, class ASType>
class BlockedExtrapolator : public BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>  {
protected:
    virtual void init();
    virtual void extrapolate(std::vector<Prefix<>*> *prefix_blocks, std::vector<Prefix<>*> *subnet_blocks);

public:
    BlockedExtrapolator(bool random_b=true,
                        bool invert_results=true, 
                        bool store_depref=false, 
                        std::string a=ANNOUNCEMENTS_TABLE,
                        std::string r=RESULTS_TABLE, 
                        std::string i=INVERSE_RESULTS_TABLE, 
                        std::string d=DEPREF_RESULTS_TABLE, 
                        uint32_t iteration_size=false) : BaseExtrapolator<SQLQuerierType, GraphType, AnnouncementType, ASType>(random_b, invert_results, store_depref, a, r, i, d, iteration_size) {
        
    }
    
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
    virtual void populate_blocks(Prefix<>*, 
                                    std::vector<Prefix<>*>*, 
                                    std::vector<Prefix<>*>*);

    /** Process a set of prefix or subnet blocks in iterations.
    */
    virtual void extrapolate_blocks(uint32_t &announcement_count, 
                                    int &iteration, 
                                    bool subnet, 
                                    std::vector<Prefix<>*> *prefix_set);

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
    virtual void give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp = 0);

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