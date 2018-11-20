#ifndef AS_H
#define AS_H

#include "Announcement.h"
#include <string>
#include <vector>
#include <iostream>

struct AS {
    uint32_t asn;
    bool visited;
    std::vector<uint32_t> providers; 
    std::vector<uint32_t> peers; 
    std::vector<uint32_t> customers; 

    AS();
    AS(uint32_t myasn);
    void setASN(uint32_t myasn);
    void printDebug();

};

#endif
