#ifndef AS_GRAPH_H
#define AS_GRAPH_H

#include <type_traits>
#include <iostream>

#include "Announcement.h"

template<class T>
class ASGraph {

static_assert(std::is_base_of<Announcement, T>::value, "T must inherit from Announcement");

public:
    T announcement;

    ASGraph(T announcement) : announcement(announcement) {

    }

    void print() {
        std::cout << announcement.origin << std::endl;
    }
};

#endif