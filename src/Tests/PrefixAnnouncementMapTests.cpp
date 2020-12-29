#include "PrefixAnnouncementMap.h"

bool prefixAnnouncementMap_test_insert() {
    PrefixAnnouncementMap<Announcement> map(10);

    Prefix<> p(0x89630000, 0xFFFF0000, 0, 0);
    Announcement ann(13796, p, 22742);

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

    auto search = map.find(p);

    if(search->origin != 13796) {
        std::cerr << "Announcement was not inserted correctly!" << std::endl;
        return false;
    }

    return true;
}