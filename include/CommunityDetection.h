#ifndef COMMUNITY_DETECTION_H
#define COMMUNITY_DETECTION_H

#include <unordered_map>
#include <unordered_set>

#include "Announcements/EZAnnouncement.h"
#include "Graphs/EZASGraph.h"

/**
 *  For EZBGPsec only!
 * 
 *  Community Detection is responsible for collecting the reports from the ASes in the graph to deduce suspect ASes
 *  and remove ASes at the end of the round if they have been proven to be malicious. 
 */
class CommunityDetection {
public:
    class Component {
    private: 
        uint32_t minimum_vertex_cover_helper(uint32_t root_asn, std::vector<std::vector<uint32_t>> hyper_edges_to_find);
        uint32_t local_minimum_vertex_cover_helper(uint32_t root_asn, std::vector<std::vector<uint32_t>> hyper_edges_to_find);

    public:
        std::vector<std::vector<uint32_t>> hyper_edges;

        //Search up an AS's degree count
        std::unordered_map<uint32_t, uint32_t> as_to_degree_count;

        //Search for all ASes with a certain degree count
        std::unordered_map<uint32_t, std::set<uint32_t>> degree_sets;

        //Since an AS can only be in one component, the unique identifier will be one of the ASns
        uint32_t unique_identifier;

        Component(std::vector<uint32_t> &hyper_edge);
        ~Component();

        /**
         *  Finds out if a given hyper edge has already been added to this compoonent
         * 
         *  @param hyper_edge -> The hyper edge to check for
         *  @return A boolean expressing if this component contains the hyper edge
         */
        bool contains_hyper_edge(std::vector<uint32_t> &hyper_edge);

        void change_degree(uint32_t asn, bool increment);

        /**
         *  Given the hyper edge, this function will add it to the component.
         *  This will increment the degree count of the asn's in this hyper edge and move them to the appropriate 
         *  This assumes that it is appropiate to add the hyper edge. You should check if this edge would cause a merge first, and also do that merge.
         * 
         * @param hyper_edge -> The hyper edge to be added to this component
         */
        void add_hyper_edge(std::vector<uint32_t> &hyper_edge);

        /**
         *  This will take all of the hyper edges of the other component and add them to this one.
         *  This will not alter the other component, nor will its memory be freed or deleted.
         */
        void merge(Component *other);

        /**
         * Given an ASN, calculate the minimum number of nodes it must use to cover all of the hyper edges that it is in
         * 
         * The smallest amount of nodes needed to cover the hyper edges that this node is in, however this node is not allowed to be used.
         * 
         * @param asn -> The AS to start from
         * @return -> The minimum nodes to cover all hyperedges that the AS is in. 0 If the AS is not in the component
         */
        uint32_t minimum_vertex_cover(uint32_t asn);

        /**
         * Given an ASN, calculate the minimum vertex cover similar to minimum_vertex_cover. 
         * However, the only difference here is that the ASN's that can be eliminated here are *neighbors* of the AS being measured
         * 
         * @param asn -> The AS to measure and to take the neighbors of
         * @return -> The minimum amount of neighbors that have to be malicious in order for the reports to have occured.
         */
        uint32_t local_minimum_vertex_cover(uint32_t asn);

        void remove_hyper_edge(std::vector<uint32_t> &hyper_edge);

        void remove_AS(uint32_t asn_to_remove);

        void threshold_filtering(CommunityDetection *community_detection, EZASGraph *graph);

        void virtual_pair_removal(CommunityDetection *community_detection, EZASGraph *graph);
    };

public:
    uint32_t threshold;
    std::unordered_map<uint32_t, Component*> identifier_to_component;

    CommunityDetection(uint32_t threshold);
    virtual ~CommunityDetection();

    /**
     *  Clears all components of hyper edges. Complete clean slate. Components are deleted from memory.
     *  However, the data structures themselves will remain in memory. Thus, this object can continue
     *  to be used after calling clear. This function just empties out the graph. That is all.
     */
    void clear();

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
     * This will simply go through the components of the hyper graph and remove amy AS that has a degree above the threshold.
     */
    void threshold_filtering(EZASGraph *graph);

    void virtual_pair_removal(EZASGraph *graph);
};
#endif