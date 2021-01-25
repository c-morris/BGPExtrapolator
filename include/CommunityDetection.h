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
        /**
         * This helper function takes in the hyper edges that the root_asn is in. It then will find the minimum amount of AS removals to remove all hyper edges
         * 
         * The idea is to pick an AS that is in any of these edges, and is not the root_asn, and remove all edges that contain that AS.
         * This is done recursively in a brute forced way to calculate the minimum number of ASes needed to remove all the edges.
         * 
         * TODO: make this not brute-forced
         * 
         * @param root_asn -> The AS to measure the mvc of
         * @param hyper_edges_to_find -> A list of the hyper edges that the root_asn is part of that must be recursively removed from
         * @param local -> Denotes whether or not the allowed ASes to be removed from the edges must be a neighbor to the root_asn (in this graph)
         */
        uint32_t minimum_vertex_cover_helper(uint32_t root_asn, std::vector<std::vector<uint32_t>> hyper_edges_to_find, bool local, uint32_t thresh, uint32_t best_so_far, uint32_t depth);

        /**
         * This will increment or decrement the degree of an AS, and update all relevant data structures. If the degree goes to 0, it will be removed from the graph.
         * 
         * If the AS is not part of the component and increment is truel, then the AS will be added into the structures
         * 
         * @param asn -> The asn to change the degree of
         * #param increment -> True to increment the degree, false to decrease the degree
         */
        void change_degree(uint32_t asn, bool increment);

    public:
        //TODO: This may benefit from being a map from asn to hyperedge? This kind of search is done often
        std::vector<std::vector<uint32_t>> hyper_edges;

        //Search up an AS's degree count
        std::unordered_map<uint32_t, uint32_t> as_to_degree_count;

        //Search for all ASes with a certain degree count
        std::unordered_map<uint32_t, std::set<uint32_t>> degree_sets;

        //Since an AS can only be in one component, the unique identifier will be one of the ASns
        uint32_t unique_identifier;

        /**
         * Initializes this component with this hyper edge
         * Every AS in the edge is granted a degree of 1
         */
        Component(std::vector<uint32_t> &hyper_edge);
        ~Component();

        /**
         *  Finds out if a given hyper edge has already been added to this compoonent
         * 
         *  @param hyper_edge -> The hyper edge to check for
         *  @return A boolean expressing if this component contains the hyper edge
         */
        bool contains_hyper_edge(std::vector<uint32_t> &hyper_edge);

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
         * 
         *  YOU MUST REMOVE THE OTHER COMPONENT FROM THE COMMUNITY DETECTION OBJECT AFTER THIS IS CALLED.
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
        uint32_t minimum_vertex_cover(uint32_t asn, uint32_t thresh=5);

        /**
         * Given an ASN, calculate the minimum vertex cover similar to minimum_vertex_cover. 
         * However, the only difference here is that the ASN's that can be eliminated here are *neighbors* of the AS being measured
         * 
         * @param asn -> The AS to measure and to take the neighbors of
         * @return -> The minimum amount of neighbors that have to be malicious in order for the reports to have occured.
         */
        uint32_t local_minimum_vertex_cover(uint32_t asn, uint32_t thresh=5);

        /**
         * This takes in a hyper edge and removes each AS in the edge
         * 
         * @param hyper_edge -> List of ASes to remove from the component
         */
        void remove_hyper_edge(std::vector<uint32_t> &hyper_edge);

        /**
         * Completely removes any trace of this AS.
         * All edges containing this component will be removed. All relevant ASes will have a decrement to their degree.
         * 
         * This AS will result in a degree of 0 and as such will be completely removed from the data scructures
         */
        void remove_AS(uint32_t asn_to_remove);

        void local_threshold_filtering(CommunityDetection *community_detection);

        /**
         * Given the current state of the component, this will filter out ASes as attackers with local and global mvc
         * 
         * @param community_detection -> Pointer to the community detection object since C++ does not give implicit refrence to the outer class
         * @param graph -> EZASGraph to remove attackers from if they are detected
         */
        void global_threshold_filtering(CommunityDetection *community_detection);

        /**
         * This will recursievely go through all permutations of edge removal and perform threshold filtering on every permutation to dig out any additional attackers
         * 
         * @param community_detection -> Pointer to the community detection object since C++ does not give implicit refrence to the outer class
         * @param graph -> EZASGraph to remove attackers from if they are detected
         */
        void virtual_pair_removal(CommunityDetection *community_detection);

        /**
         * Approximation for local MVC that runs in polynomial time.
         * 
         * @param community_detection -> Pointer to the community detection object 
         */
        void local_threshold_approx_filtering(CommunityDetection *community_detection);
    };

public:
    uint32_t local_threshold;
    uint32_t global_threshold;

    //TODO: The identifier is currently based on the first ASN in the first hyper edge that the component is initialized with. This can brake. Do better.
    std::unordered_map<uint32_t, Component*> identifier_to_component;

    std::set<std::vector<uint32_t>> blacklist_paths;
    std::unordered_set<uint32_t> blacklist_asns;

    std::vector<std::vector<uint32_t>> edges_to_proccess;

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
    CommunityDetection(uint32_t local_threshold);
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
     * This will go through the components of the hyper graph and remove any AS from the graph that has an mvc above the threshold. 
     * 
     * See paper on the details on how this is done with global and local mvc's
     * 
     * @param graph -> The graph to remove from
     */
    void global_threshold_filtering();

    void local_threshold_filtering();

    /**
     * For each component, recursively compute every permutation of removing edges from the component. 
     * For each permutation, perform threshold filtering to see what additional attackers are present.
     * 
     * @param graph -> The graph to remove from
     */
    void virtual_pair_removal();

    void process_reports(EZASGraph *graph);
};
#endif
