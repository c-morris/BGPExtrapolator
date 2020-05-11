#include "RovASGraph.h"

RovASGraph::RovASGraph(RovAnnouncement announcement) : ASGraph(announcement) {

}

void RovASGraph::print() {
    std::cout << announcement.origin << ", " << announcement.rovData << std::endl;
}