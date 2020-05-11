#ifndef ROV_AS_GRAPH
#define ROV_AS_GRAPH

#include <iostream>

#include "RovAnnouncement.h"
#include "ASGraph.h"

class RovASGraph : public ASGraph<RovAnnouncement> {
public:
    RovASGraph(RovAnnouncement announcement);

    void print();
};

#endif