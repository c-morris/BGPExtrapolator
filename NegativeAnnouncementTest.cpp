#include <iostream>
#include "Tests.h"
#include "NegativeAnnouncement.h"

bool test_nannouncement_constructor() {
    Prefix<> p("137.99.0.0", "255.255.0.0");
    Prefix<> s("137.99.0.0", "255.255.255.0");
    NegativeAnnouncement n = NegativeAnnouncement(13796, p.addr, p.netmask, 22742, std::set<Prefix<>>());
    n.null_route_subprefix(s);
    std::cerr << n;
    return true;
}
