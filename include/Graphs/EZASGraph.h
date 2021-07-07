#ifndef EZ_AS_GRAPH_H
#define EZ_AS_GRAPH_H

#include "ASes/EZAS.h"
#include "Graphs/BaseGraph.h"
#include "SQLQueriers/EZSQLQuerier.h"

class EZASGraph : public BaseGraph<EZAS> {
public:
    EZASGraph();
    ~EZASGraph();

    EZAS* createNew(uint32_t asn);

    /**
     *  Given some asn (which has been determined to be malicious) and disconnect it from any adopting neighbors
     *  Make sure to re-process the graph after doing this because the components of the graph may not be the same after this removal
     *  Don't run this in the middle of the round either. It may invalidate the connected components of the graph
     * 
     *  @param asn -> The suspect AS that adopters should disconnect from
     */
    void disconnect_as_from_adopting_neighbors(uint32_t asn);
    
    void process(SQLQuerier<>* querier);

    /** Load EzBGPsec policy assignments from the database and assign them to ASes.
     */  
    void assign_policies(EZSQLQuerier* querier);
};
#endif
