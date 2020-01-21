#ifndef ASPATH_H
#define ASPATH_H

#include <vector>
#include <cstring>
#include <bits/stdc++.h>

#include "Logger.h"
#include "Prefix.h"
#include "AS.h"
#include "Announcement.h"
#include "ASGraph.h"

class ASPath {
    private:
        std::vector<uint32_t>* as_path;

    public:
        /**
        *   Creates an empty path
        */
        ASPath();

        /** 
        * Parse array-like strings from db to get AS_PATH in a vector.
        *
        * libpqxx doesn't currently support returning arrays, so we need to do this. 
        * path_as_string is passed in as a copy on purpose, since otherwise it would
        * be destroyed.
        *
        * @param path_as_string the as_path as a string returned by libpqxx
        */
        ASPath(std::string path_as_string);

        /** Seed announcement on all ASes on as_path. 
        *
        * The from_monitor attribute is set to true on these announcements so they are
        * not replaced later during propagation. 
        *
        * @param graph The ASGraph of the ASes in the system
        * @param prefix The prefix this announcement is for.
        * @param timestamp Timestamp for tie-breaking
        */
        void seedAnnouncements(ASGraph* graph, Prefix<> prefix, int64_t timestamp, bool random);

        /**
        *   Detects if there is a loop in the AS path
        */
        bool containsLoop();

        /**
        *   Returns the integer representation of the AS path
        */
        std::vector<uint32_t>* getASPath();
};
#endif