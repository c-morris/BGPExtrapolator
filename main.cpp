#include "RovAnnouncement.h"
#include "RovASGraph.h"

int main() {
    RovAnnouncement announcement(5, 10);

    RovASGraph graph(announcement);

    graph.print();
}