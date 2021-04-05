#ifndef COMMUNITY_DETECTION_H
#define COMMUNITY_DETECTION_H

#include <limits.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "Announcements/EZAnnouncement.h"
#include "Graphs/EZASGraph.h"

//Forward declaration to handle circular dependency
class EZExtrapolator;

/**
 *  For EZBGPsec only!
 * 
 *  Community Detection is responsible for collecting the reports from the ASes in the graph to deduce suspect ASes
 *  and remove ASes at the end of the round if they have been proven to be malicious. 
 */
class CommunityDetection {
private:
    std::vector<std::vector<uint32_t>> edges_to_process;

public:
    EZExtrapolator *extrapolator;

    std::set<std::vector<uint32_t>> blacklist_paths;
    std::unordered_set<uint32_t> blacklist_asns;

    std::vector<std::vector<uint32_t>> hyper_edges;

    uint32_t local_threshold;

    /**
     * This will simply store this threshold into a member variable.
     * 
     * Hyper edges can be added by adding reports (EZAnnouncements that are from an attacker). 
     * This class will handle detecting if the MAC is truly visible and condensing the path down to the relevant segment for community detection.
     * 
     * This class will separate the hyper edges (paths detected invalid MACs) into graph components (see the paper for visuals on what a hyper edge looks like)
     * Components are handled internall in this class that way internal component-wise operations can be performed.
     * Components will merge together when a reported path connects them.
     * 
     * In addition, the number of hyper edges an AS is in is referred to as its degree. This is also tracked as edges are added.
     * The end goal is for every component to filter out attackers with threshold filtering an perform virtual removals to filter out more attackers 
     *      (see paper for conceptual details) 
     */
    CommunityDetection(EZExtrapolator *extrapolator, uint32_t local_threshold);

    /**
     * Checks whether a given hyper edge is already in the graph
     * 
     * @param hyper_edge -> The hyper edge to search for
     * @return whether or not this hyper edge is in the graph
     */
    bool contains_hyper_edge(std::vector<uint32_t> &hyper_edge);

    /**
     *  DO NOT CONFUSE THIS WITH ADD_REPORT!!!!
     * 
     *  This assumes that this vector contains a VISIBLE invalid MAC, 
     *      and that this is the most reduced form of the edge.
     * 
     *  The difference between add_report and add_hyper_edge is that add_report takes in an
     *      announcement and extracts the visible hyper edges, 
     *      which are added to the graph through this function
     * 
     *  Given a hyper edge, add it to the graph. 
     *  If the hyper edge is already in a component, then it will not be added.
     *  Otherwise, it will check if any AS on the hyper edge is already in a component.
     * 
     *  If none, then it will create a new component for this edge to occupy.
     *  If there is only 1 related component, then this hyper edge will be added to that component.
     *  If this hyper edge connects 2 or more components, they will all be merged into one component,
     *      and this edge will be added to that component
     * 
     *  @param hyper_edge -> The hyper edge that contains a visible invalid MAC
     */
    void add_hyper_edge(std::vector<uint32_t> &hyper_edge);

    /**
     * An attacking announcement that an adopting EZBGPsec AS accepted. In theory, this announcement has an invalid MAC for the origin.
     * This will extract the hyper edge(s) from the announcement path (if any are visible).
     * It will take each of these hyper edges and add them to the hyper graph. Thereby incrementing their degree count in the hypergraph.
     * 
     * The EZASGraph is needed to check for invalid MACs and check who is an adopter. No modifications to the graph are made here.
     * 
     * @param announcement -> the malicious announcement to extract hyper edges from
     * @param graph -> ASGraph to "confirm MACs" and get AS infromation from. 
     */
    void add_report(EZAnnouncement &announcement, EZASGraph *graph);

    /**
     * Test if the given ASes in sprime form a cover over s.
     *
     * @return true if sprime covers s, false otherwise.
     */
    bool is_cover(std::set<uint32_t> sprime, std::set<uint32_t> s);

    /**
     * Generate a map of ASNs to other ASNs which are indistinguishable from them.
     *
     * @return the map described above 
     */
    std::map<uint32_t, std::set<uint32_t>> gen_ind_asn(std::set<uint32_t> s, const std::vector<std::vector<uint32_t>> &edges);

    /**
     * Generate a map of ASNs to its degree in this set of edges.
     *
     * @return the map described above 
     */
    std::map<uint32_t, uint32_t> get_degrees(std::set<uint32_t> s, const std::vector<std::vector<uint32_t>> &edges);

    /**
     * Generate the universal set of ASNs that exist in a vector of edges.
     *
     * @return the set of unique ASNs that exist in a list of edges
     */
    std::set<uint32_t> get_unique_asns(std::vector<std::vector<uint32_t>> edges);


    std::vector<std::set<uint32_t>> gen_cover_candidates(size_t sz, 
        std::map<uint32_t, std::set<uint32_t>> &ind_map,
        std::map<uint32_t, uint32_t> &degrees);

    // Deprecated
    void local_threshold_approx_filtering();


    void process_reports(EZASGraph *graph);
};
#endif
