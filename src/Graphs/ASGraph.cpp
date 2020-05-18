#include "Graphs/ASGraph.h"

ASGraph::ASGraph() {

}

ASGraph::~ASGraph() {

}

AS* ASGraph::createNew(uint32_t asn) {
    return new AS(asn, inverse_results);
}