#ifndef EZ_EXTRAPLATOR_H
#define EZ_EXTRAPLATOR_H

#define DEFAULT_NUM_ASES_BETWEEN_ATTACKER 0
#define DEFAULT_COMMUNITY_DETECTION_THRESHOLD 0
#define DEFAULT_POLICY_TABLES NULL

#include "Extrapolators/BlockedExtrapolator.h"

#include "SQLQueriers/EZSQLQuerier.h"
#include "Graphs/EZASGraph.h"
#include "Announcements/EZAnnouncement.h"
#include "ASes/EZAS.h"
#include "CommunityDetection.h"

/**
 * The idea here is to estimate the probability that an "edge" AS can pull off an attacck
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

    /*   - Attacker gets the traffic: successful attack
     *   - Origin gets the traffic: successful connection
     *   - Nobody gets the traffic: disconnection
     */

    uint32_t num_rounds; // Number of rounds in simulation
    uint32_t round; // Current round

    //Number of "ASes" between the attacker and the origin
    uint32_t num_between;

    EZExtrapolator(bool random_tiebraking,
                    bool store_invert_results, 
                    bool store_depref_results, 
                    std::string announcement_table,
                    std::string results_table, 
                    std::string inverse_results_table, 
                    std::string depref_results_table, 
                    std::string config_section,
                    std::vector<std::string> *policy_tables, 
                    uint32_t iteration_size,
                    uint32_t num_rounds,
                    uint32_t num_between,
                    uint32_t community_detection_threshold,
                    int exclude_as_number,
                    uint32_t mh_mode);
    
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
     * This will find the neighbor to the attacker on the AS path.
     * The initial call should have the as be the victim.
     * This fucntion will likely get removed in the future because of path propagation.
     */
    uint32_t getPathNeighborOfAttacker(EZAS* as, Prefix<> &prefix, uint32_t attacker_asn);

    /**
     * This runs at the end of every iteration
     * 
     * Baically, go to the victim and see if it chose the attacker route.
     * if the prefix didn't reach the victim (an odd edge case), then it does not count to the total
     * If the Victim chose the fake announcement path, then sucessful attack++
     * 
     * In addition, if there was a successful attack, record the edge (asn pair) from the attacker to the neighbor on the path
     */
    void calculate_successful_attacks();

    /*
    * A quick overwrite that removes the traditional saving functionality since we are 
    *   only interested in the probibilities, not so much the actual info the extrapolator dumps out about the accepted announcements.
    * Comment out the first line if the output is needed.
    */
    void save_results(int iteration);
};

#endif
