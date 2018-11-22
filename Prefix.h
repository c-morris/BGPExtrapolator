#ifndef PREFIX_H
#define PREFIX_H

struct Prefix {
    uint32_t addr;
    uint32_t netmask;
    // comparison operators for maps
    // comparing the addr first ensures the more specific address is greater
    bool operator<(const Prefix &b) const {
        if (addr < b.addr) {
            return true;
        } else if (netmask < b.netmask) {
            return true;
        } else {
            return false;
        }
    }
    bool operator==(const Prefix &b) const {
        return !(*this < b || b < *this);
    }
    bool operator>(const Prefix &b) const {
        return !(*this < b || *this == b);
    }
};

#endif

