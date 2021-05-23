#include "PrefixAnnouncementMap.h"

bool prefixAnnouncementMap_test_insert() {
    PrefixAnnouncementMap<Announcement<>> map(10);

    Prefix<> p(0x89630000, 0xFFFF0000, 0, 0);
    Announcement<> ann(13796, p, 22742);

    if(map.find(p) != map.end()) {
        std::cerr << "Equality of not-filled announcement and the end iterator is not working!" << std::endl;
        return false;
    }

    for(auto &ann : map) {
        if(ann.tstamp != -1) {
            std::cerr << "Not all announcements are empty when the map is initialized!" << std::endl;
            return false;
        }
    }

    map.insert(p, ann);

    bool validAnn = false;
    for(auto &ann : map) {
        if(ann.tstamp != -1) {
            if (validAnn) {
                std::cerr << "Multiple announcements found, but only one was added." << std::endl;
                return false;
            }
            validAnn = true;
        } else {
            std::cerr << "Received an unexistent (timestamp = -1) announcement during iteration." << std::endl;
            return false;
        }
    }
    if (!validAnn) {
        std::cerr << "No announcements found, even though one was added." << std::endl;
        return false;
    }

    auto search = map.find(p);

    if(search->origin != 13796) {
        std::cerr << "Announcement was not inserted correctly!" << std::endl;
        return false;
    }

    return true;
}