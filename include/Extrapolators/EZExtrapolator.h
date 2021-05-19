#ifndef EZ_EXTRAPLATOR_H
#define EZ_EXTRAPLATOR_H

#define DEFAULT_COMMUNITY_DETECTION_LOCAL_THRESHOLD 3
#define DEFAULT_POLICY_TABLES NULL
#define HIJACKED 64513
#define NOTHIJACKED 64514

#include <sstream>

#include "Extrapolators/BlockedExtrapolator.h"

#include "SQLQueriers/EZSQLQuerier.h"
#include "Graphs/EZASGraph.h"
#include "Announcements/EZAnnouncement.h"
#include "ASes/EZAS.h"
#include "TableNames.h"
#include "CommunityDetection.h"

/**
 * The idea here is to estimate the probability that an "edge" AS can pull off an attack
 * 
 * "Edge" AS: an AS with at least one provider and no customers.
 * 
 * Optimization: don't save results to the db, we aren't interested in them
 * 
 * The attack:
 *      Origin: The AS that is the origin to the prefix 
 *      Attacker: sends an announcement that it is the provider of the origin, for this prefix  
 *      Victim: This AS is used at the end to see what path it chose, and whether is contains the attacker
 *  
 * Trace back:
 *      The EZAnnnouncement has a flag for whether the announcement is from an attacker. This is carried
 *          over in the copy constructor. 
 *      Then, after propagation, all we have to do is chack if the announcement the victim AS has
 *          for the prefix is from an attacker
 * 
 * Disconnections:
 *      If an attack is successful, at the end of the round (where a round is a full propagation throughout the graph)
 *      we disconnect the edge to the attack on the path. Imagine this as the neighbor detecting (through ezBGPsec) 
 *      that it is connected to an attacker. It will disconnect then. 
 * 
 *      We then redo the propagation, but with those edges removed to see how many attacks presist as edges are removed
 *      This is then repeated tuntil there are no more successful attacks (which is guaranteed to terminate)
 * 
 * IMPORTANT: This extrapolator eats RAM like it is going out of style. Run on low iteration sizes (4,000 at most)
 *      That is if the iterations contain a dense amount of announcements from the origin.
 *      Because we have to save the prefix it announces to "trace back" later.
 */
class EZExtrapolator : public BlockedExtrapolator<EZSQLQuerier, EZASGraph, EZAnnouncement, EZAS> {
public:
    CommunityDetection *communityDetection;
    std::set<std::pair<Prefix<>,uint32_t>> attacker_prefix_pairs;

    /*   - Attacker gets the traffic: successful attack
     *   - Origin gets the traffic: successful connection
     *   - Nobody gets the traffic: disconnection
     */

    uint32_t num_rounds; // Number of rounds in simulation
    uint32_t round; // Current round
    uint32_t next_unused_asn; // Unused ASNs to populate attacker announcements

    EZExtrapolator(bool random_tiebraking,
                    bool store_results, 
                    bool store_invert_results, 
                    bool store_depref_results, 
                    std::string announcement_table,
                    std::string results_table, 
                    std::string inverse_results_table, 
                    std::string depref_results_table, 
                    std::string full_path_results_table, 
                    std::vector<std::string> *policy_tables, 
                    std::string config_section,
                    uint32_t iteration_size,
                    uint32_t num_rounds,
                    uint32_t community_detection_threshold,
                    int exclude_as_number,
                    uint32_t mh_mode,
                    bool origin_only,
                    std::vector<uint32_t> *full_path_asns,
                    int max_threads);
    
    EZExtrapolator(uint32_t community_detection_threshold);

    EZExtrapolator();
    ~EZExtrapolator();

    void init();

    /**
     *  Every iteration, the ASes make their reports and those reports are gathered
     */
    void gather_community_detection_reports();

    void perform_propagation();

    /**
     * This is where the attacker announcement is sent out. All paths are seeded and if an origin of the path
     *      is to be attacked, then have the attacker process a malicous announcement and send it out (muahahaa).
     */
    void give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp = 0);


    /**
     * Return a never-before-seen ASN
     */
    uint32_t get_unused_asn();

    /*
    * A quick overwrite that removes the traditional saving functionality since we are 
    *   only interested in the probibilities, not so much the actual info the extrapolator dumps out about the accepted announcements.
    * Comment out the first line if the output is needed.
    */
    void save_results(int iteration);

    void save_results_round(int iteration);
};

#endif
