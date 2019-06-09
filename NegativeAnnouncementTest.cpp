#include <iostream>
#include "Tests.h"
#include "NegativeAnnouncement.h"

bool test_nannouncement_constructor() {
    Announcement ann = Announcement(13796, 0x01010101, 0xffff0000, 2.62, 222, false);
    Prefix<> p("137.99.0.0", "255.255.0.0");
    Prefix<> s("137.99.0.0", "255.255.255.0");
    // NegativeAnnouncement n = NegativeAnnouncement(123, p.addr, p.netmask, 22742, std::set<Prefix<>>());
    NegativeAnnouncement n = NegativeAnnouncement(123, p.addr, p.netmask, 3, 22742, false, std::set<Prefix<>>(), ann);
    n.null_route_subprefix(s);
    //std::cerr << n;
    return true;
}
