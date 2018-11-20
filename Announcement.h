#ifndef ANNOUNCEMENT_H
#define ANNOUNCEMENT_H

#include <cstdint>
#include <iostream>

struct Announcement {
    uint32_t prefix; // encoded with subnet mask
    uint32_t netmask; // subnet mask
    uint32_t origin; // origin ASN
    double priority; 
    uint32_t received_from_asn;

    Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t from_asn) {
        prefix = aprefix;
        netmask = anetmask;
        origin = aorigin;
        received_from_asn = from_asn;
        priority = 0.0;
    }

    //friend std::ostream& operator<<(std::ostream &os, const Announcement& ann);
    friend std::ostream& operator<<(std::ostream &os, const Announcement& ann) {
        os << "Prefix:\t\t" << std::hex << ann.prefix << " & " << std::hex << 
            ann.netmask << std::endl << "Origin:\t\t" << std::dec << ann.origin
            << std::endl << "Priority:\t" << ann.priority << std::endl 
            << "Recv'd from:\t" << std::dec << ann.received_from_asn;
        return os;
    }
};


#endif
