#ifndef AS_GRAPH_H
#define AS_GRAPH_H

#include "Graphs/BaseGraph.h"

class ASGraph : public BaseGraph<AS> {
public:
    AS* createNew(int asn);

    ASGraph();
    ~ASGraph();
};

#endif