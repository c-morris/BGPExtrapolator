#include <stdlib.h>
#include <iostream>
#include <time.h>
#include "AS.h"
#include "ASGraph.h"

ASGraph* ran_graph(int num_edges, int num_vertices) {
    ASGraph* graph = new ASGraph;
    //srand(time(NULL));
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


bool cyclic_util(ASGraph* graph, int asn, std::map<uint32_t, bool>* visited, std::map<uint32_t, bool>* recStack) { 
    if (visited->find(asn)->second == false) {
        visited->find(asn)->second = true;
        recStack->find(asn)->second = true;
        AS* cur_AS = graph->ases->find(asn)->second;
        for (auto &provider_pair : *cur_AS->providers) {
            if (!visited->find(provider_pair.first)->second &&
                cyclic_util(graph, provider_pair.first, visited, recStack)) {
                return true;
            } else if (recStack->find(provider_pair.first)->second) {
                return true;
            }
        }
    }
    recStack->find(asn)->second = false;
    return false;
}


bool is_cyclic(ASGraph* graph) {
    auto visited = new std::map<uint32_t, bool>;
    auto recStack = new std::map<uint32_t, bool>;
    
    for (auto const& as : *graph->ases) {
        visited->insert(std::pair<uint32_t, bool>(as.first, false));
        recStack->insert(std::pair<uint32_t, bool>(as.first, false));
    } 
  
    // Call the recursive helper function to detect cycle in different 
    for (auto const& as : *graph->ases) {
        if (cyclic_util(graph, as.first, visited, recStack)) 
            return true;
    }
  
    return false; 
}
