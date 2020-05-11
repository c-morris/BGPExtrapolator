#include "RovAnnouncement.h"
#include "RovASGraph.h"

#include "Announcement.h"
#include "ASGraph.h"

#include <iostream>

int main() {
    Announcement announcement(5);
    ASGraph graph(announcement);
    graph.print();

    RovAnnouncement rovAnnouncement(1, 2);
    RovASGraph rovGraph(rovAnnouncement);
    rovGraph.print();
}