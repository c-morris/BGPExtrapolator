#ifndef ROVPP_AS_GRAPH_H
#define ROVPP_AS_GRAPH_H

// #include "Graphs/BaseGraph.h"
#include "ASes/ROVppAS.h"

#include "SQLQueriers/ROVppSQLQuerier.h"
// #include "SQLQueriers/SQLQuerier.h"

class ROVppASGraph : public BaseGraph<ROVppAS> {
public:
    // Sets of ASNs to keep track of attackers and victims
    // TODO need set of vectors for policy index
    std::set<uint32_t> *attackers;
    std::set<uint32_t> *victims;
    
    ROVppASGraph();
    ~ROVppASGraph();

    ROVppAS* createNew(uint32_t asn);

    // Overriden Methods
    void process(SQLQuerier *querier);
    void create_graph_from_db(ROVppSQLQuerier*);

    // Overloaded 
    void to_graphviz(std::ostream &os, std::vector<uint32_t> asns);
    void to_graphviz_traceback(std::ostream &os, uint32_t asn, int depth);
};

#endif