#ifndef EZ_EXTRAPLATOR_H
#define EZ_EXTRAPLATOR_H

#include "Extrapolators/BlockedExtrapolator.h"

#include "SQLQueriers/EZSQLQuerier.h"
#include "Graphs/EZASGraph.h"
#include "Announcements/EZAnnouncement.h"
#include "ASes/EZAS.h"

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
    double total_attacks;
    double successful_attacks;

    EZExtrapolator(bool random = true,
                    bool invert_results = true, 
                    bool store_depref = false, 
                    std::string a = ANNOUNCEMENTS_TABLE,
                    std::string r = RESULTS_TABLE, 
                    std::string i = INVERSE_RESULTS_TABLE, 
                    std::string d = DEPREF_RESULTS_TABLE, 
                    uint32_t iteration_size = false);
                    
    ~EZExtrapolator();

    void init();

    void perform_propagation();
    void propagation_drop_connections_helper(std::vector<Prefix<>*> *prefix_blocks, std::vector<Prefix<>*> *subnet_blocks);

    void give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp = 0);
    uint32_t getPathNeighborOfAttacker(EZAS* as, Prefix<> &prefix, uint32_t attacker_asn);
    void calculate_successful_attacks();
    void save_results(int iteration);
};

#endif