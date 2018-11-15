#include <cstdint>

#ifndef ANNOUNCEMENT_H
#define ANNOUNCEMENT_H

struct Announcement {
    uint32_t prefix; // encoded with subnet mask
    uint32_t netmask; // subnet mask
    uint32_t origin; // origin ASN
    double priority; 
    uint32_t received_from_asn;
};
#endif
