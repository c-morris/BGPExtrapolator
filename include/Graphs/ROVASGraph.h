#ifndef ROV_AS_GRAPH_H
#define ROV_AS_GRAPH_H

#include "ASes/ROVAS.h"
#include "Graphs/BaseGraph.h"
#include "SQLQueriers/ROVSQLQuerier.h"

class ROVASGraph : public BaseGraph<ROVAS> {
public:
    ROVASGraph(bool store_inverse_results, bool store_depref_results);
    ROVASGraph();
    ~ROVASGraph();
    ROVAS* createNew(uint32_t asn);

    /** Process the graph without removing stubs (needs querier to save them).
     * 
     * @param querier
    */
    void process(SQLQuerier<> *querier);

    /** Generates an ROVASGraph from relationship data in an SQL database based upon:
     *      1) A populated peers table
     *      2) A populated customer_providers table
     *      3) Populated policy tables
     * 
     * @param querier
     */
    void create_graph_from_db(ROVSQLQuerier *querier);

    /** The asn passed in this function will be added to the list of attackers
     * 
     * @param asn AS number of an attacker
     */
    void add_attacker(uint32_t asn);

private:
    // Set of ASNs to keep track of attackers
    std::set<uint32_t> *attackers;
};
#endif
