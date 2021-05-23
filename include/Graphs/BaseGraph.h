/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#ifndef BASE_GRAPH_H
#define BASE_GRAPH_H

#include <map>
#include <unordered_map>
#include <vector>
#include <stack>
#include <sys/stat.h>
#include <dirent.h>
#include <pqxx/pqxx>

#include "ASes/AS.h"
#include "ASes/EZAS.h"
#include "ASes/ROVppAS.h"
#include "ASes/ROVAS.h"

#include "SQLQueriers/SQLQuerier.h"
#include "TableNames.h"

template <class ASType, typename PrefixType = uint32_t>
class BaseGraph {

// static_assert(std::is_base_of<BaseAS, ASType>::value, "ASType must inherit from BaseAS");
// static_assert(std::is_convertible<ASType*, BaseAS*>::value, "ASType must inherit BaseAS as public");

public:
    std::unordered_map<uint32_t, ASType*> *ases;            // Map of ASN to AS object 
    std::vector<std::set<uint32_t>*> *ases_by_rank;     // Vector of ranks
    std::vector<std::vector<uint32_t>*> *components;    // Strongly connected components
    std::map<uint32_t, uint32_t> *component_translation;// Translate AS to supernode AS
    std::map<uint32_t, uint32_t> *stubs_to_parents;
    std::vector<uint32_t> *non_stubs;
    std::map<std::pair<Prefix<PrefixType>, uint32_t>,std::set<uint32_t>*> *inverse_results; 

    bool store_depref_results;
    // Represents the largest prefix_id in a block
    uint32_t max_block_prefix_id;

    BaseGraph(bool store_inverse_results, bool store_depref_results) {
        ases = new std::unordered_map<uint32_t, ASType*>;               // Map of all ASes
        ases_by_rank = new std::vector<std::set<uint32_t>*>;        // Vector of ASes by rank
        components = new std::vector<std::vector<uint32_t>*>;       // All Strongly connected components
        component_translation = new std::map<uint32_t, uint32_t>;   // Translate node to supernode
        stubs_to_parents = new std::map<uint32_t, uint32_t>;        // Translace stub to parent
        non_stubs = new std::vector<uint32_t>;                      // All non-stubs in the graph

        if(store_inverse_results) 
            inverse_results = new std::map<std::pair<Prefix<PrefixType>, uint32_t>, std::set<uint32_t>*>;
        else 
            inverse_results = NULL;
        
        this->store_depref_results = store_depref_results;

        // Set it to an arbitrary value to avoid changing extrapolator tests
        // The variable is changed in BlockedExtrapolator::perform_propagation
        max_block_prefix_id = 20;
    }

    virtual ~BaseGraph();

    //Creation of template type
    virtual ASType* createNew(uint32_t asn) = 0;

    //****************** Propagation Interaction ******************//

    /** Clear all announcements in AS.
    */
    virtual void clear_announcements();

    /** Translates asn to asn of component it belongs to in graph.
     *
     *  @param asn the asn to translate
     *  @return 0 if asn isn't found, otherwise return identifying ASN
     */
    virtual uint32_t translate_asn(uint32_t asn);

    //****************** Graph Setup ******************//

    /** Adds an AS relationship to the graph.
     *
     * If the AS does not exist in the graph, it will be created.
     *
     * @param asn ASN of AS to add the relationship to
     * @param neighbor_asn ASN of neighbor.
     * @param relation AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
     */
    void add_relationship(uint32_t asn, uint32_t neighbor_asn, int relation);

    /** Process with removing stubs (needs querier to save them).
    */
    virtual void process(SQLQuerier<PrefixType> *querier);

    /** Generates an ASGraph from relationship data in an SQL database based upon:
     *      1) A populated peers table
     *      2) A populated customer_providers table
     * 
     * @param querier
     */
    virtual void create_graph_from_db(SQLQuerier<PrefixType> *querier);

    /** Remove the stub ASes from the graph.
     *
     * @param querier
     */
    virtual void remove_stubs(SQLQuerier<PrefixType> *querier);

    /** Saves the stub ASes to be removed to a table on the database.
     *
     * @param querier
     */
    virtual void save_stubs_to_db(SQLQuerier<PrefixType> *querier);

    /** Saves the non_stub ASes to a table on the database.
     *
     * @param querier
     */
    virtual void save_non_stubs_to_db(SQLQuerier<PrefixType> *querier);

    /** Generate a csv with all supernodes, then dump them to database.
     *
     * @param querier
     */
    virtual void save_supernodes_to_db(SQLQuerier<PrefixType> *querier);

    /** Decide and assign ranks to all the AS's in the graph. 
     *
     *  The rank of an AS one plus is the maximum level of the nodes it has below it. 
     *  This means it is possible to have an AS of rank 0 directly below an AS of 
     *  rank 4, but not possible to have an AS of rank 3 below one of rank 2. 
     *
     *  The bottom of the DAG is rank 0. 
     */
    virtual void decide_ranks();

    //****************** Supernode Generation ******************//

    /** Tarjan driver to detect strongly connected components in the ASGraph.
     * 
     * https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
     */
    virtual void tarjan();

    /** Tarjan algorithm to detect strongly connected components in the ASGraph.
    */
    virtual void tarjan_helper(ASType *as, int &index, std::stack<ASType*> &s);

    /** Combine providers, peers, and customers of ASes in a strongly connected component.
     *  Also append to component_translation for future reference.
     */
    virtual void combine_components();

    //****************** Misc. ******************//

    /** Print all ASes for debug.
    */
    virtual void printDebug();

    /** Output python code for making graphviz digraph.
    */
    virtual void to_graphviz(std::ostream &os);

    /** Operation for debug printing AS
    */
    template <class U>
    friend std::ostream& operator<<(std::ostream &os, const BaseGraph<U>& asg);
};
#endif

