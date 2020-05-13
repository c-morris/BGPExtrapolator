#include "Graphs/ASGraph.h"

ASGraph::ASGraph() {

}

ASGraph::~ASGraph() {

}

AS* ASGraph::createNew(int asn) {
    return new AS(asn, inverse_results);
}