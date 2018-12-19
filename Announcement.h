#ifndef ANNOUNCEMENT_H
#define ANNOUNCEMENT_H

#include <cstdint>
#include <iostream>
#include "Prefix.h"

struct Announcement {
    Prefix<> prefix; // encoded with subnet mask
    uint32_t origin; // origin ASN
    double priority; 
    uint32_t received_from_asn;
    bool from_monitor = false;

    Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t from_asn) {
        prefix.addr = aprefix;
        prefix.netmask = anetmask;
        origin = aorigin;
        received_from_asn = from_asn;
        priority = 0.0;
        from_monitor = false;
    }
    Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        double pr, uint32_t from_asn, bool a_from_monitor = false) 
        : Announcement(aorigin, aprefix, anetmask, from_asn) { 
        priority = pr; 
        from_monitor = a_from_monitor;
    }
    // comparison operators for maps
    // comparing first on prefix ensures the most specific announcement gets
    // priority. The origin is not used in comparison. 
    bool operator<(const Announcement &b) const {
        if (prefix < b.prefix) {
            return true;
        } else if (priority < b.priority) { 
            return true;
        } else {
            return false;
        }
    }
    bool operator==(const Announcement &b) const {
        return !(*this < b || b < *this);
    }
    bool operator>(const Announcement &b) const {
        return !(*this < b || *this == b);
    }

    friend std::ostream& operator<<(std::ostream &os, const Announcement& ann) {
        os << "Prefix:\t\t" << std::hex << ann.prefix.addr << " & " << std::hex << 
            ann.prefix.netmask << std::endl << "Origin:\t\t" << std::dec << ann.origin
            << std::endl << "Priority:\t" << ann.priority << std::endl 
            << "Recv'd from:\t" << std::dec << ann.received_from_asn;
        return os;
    }

    std::ostream& to_csv(std::ostream &os){
        //TODO add prefix to_string that combines host and mask
        os << std::hex << prefix.addr << "," << origin << "," << priority << 
            "," << received_from_asn << std::endl;
        return os;
    }
};

#endif
