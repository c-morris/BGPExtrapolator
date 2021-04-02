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
    ROVAS* create_new(uint32_t asn);
    void process(SQLQuerier *querier);
    void create_graph_from_db(ROVSQLQuerier *querier);
    void add_attacker(uint32_t asn);

private:
    // Set of ASNs to keep track of attackers
    std::set<uint32_t> *attackers;
};
#endif