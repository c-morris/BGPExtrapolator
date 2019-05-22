#include <stdlib.h>
#include <iostream>
#include "AS.h"
#include "ASGraph.h"

ASGraph* ran_graph(int num_edges, int num_vertices) {
    ASGraph* graph = new ASGraph;

    //v = 11+rand()%10;
    //e = rand()%((v*(v-1))/2);
    int i; 
    i = 0;
    // Build a connection between two random vertex.
    while(i < num_edges) { 
        // Provider = 0, Customer = 1
        int rel_type = (rand() % 2) * 2;
        
        // Generate two random vertices
        int to = rand() % num_vertices + 1;
        int from = rand() % num_vertices + 1;

        if (to == from)
            continue;
        
        // Add to graph
        graph->add_relationship(to, from, rel_type);
        graph->add_relationship(from, to, 2 - rel_type);
        i++;
    }
    return graph;
}
