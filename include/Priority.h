#ifndef PRIORITY_H
#define PRIORITY_H

/**
 * This struct allows easy changes to priority calculation, as opposed to a singular integer
 * 
 * Longer paths have lower priority
 * 
 * relationship: Provider = 0, Peer = 1, Customer = 2, and 3 when it's the origin
 * 
 * What used to be 400 priority is now (3 << 24) + (255 << 8). 3 comes from the relationship, 
 * and 255 is the value when the path length is at its minimum (0)
*/

struct Priority {
    // Little-endian assumed
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t path_length; // Lower precedence
    uint8_t reserved3;
    uint8_t reserved4;
    uint8_t relationship; // Highest precedence
    uint8_t reserved5;
    uint8_t reserved6;

    Priority() {
        reserved1 = reserved2 = relationship 
        = reserved3 = reserved4 = reserved5 = reserved6
        = 0;
        path_length = 255;
    }

    operator uint64_t() const {
        uint64_t p = 0;
        p |= (uint64_t) reserved1;
        p |= (uint64_t) reserved2 << 8;
        p |= (uint64_t) (255 - path_length) << 16;
        p |= (uint64_t) reserved3 << 24;
        p |= (uint64_t) reserved4 << 32;
        p |= (uint64_t) relationship << 40;
        p |= (uint64_t) reserved5 << 48;
        p |= (uint64_t) reserved6 << 56;
        return p;
    };
};

#endif
