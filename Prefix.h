#ifndef PREFIX_H
#define PREFIX_H

#include <cmath>

// use uint32_t for IPv4, unsigned __int128 for IPv6
template <typename Integer = uint32_t>
struct Prefix {
    Integer addr;
    Integer netmask;
    Prefix() {}
    Prefix(std::string addr_str, std::string mask_str) {
        // TODO IPv6 Address Parsing
        // IPv4 Address Parsing
        std::string delimiter = ".";
        size_t pos = 0;
        std::string token;

        ////rename variables
        //IP to int
        uint32_t ipv4_ip_int = 0;
        int i = 0;
        std::string &s = addr_str;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            ipv4_ip_int += std::stoi(token) * std::pow(256,i++);
            s.erase(0, pos + delimiter.length());
        }
        ipv4_ip_int += std::stoi(token) * std::pow(256,i++);

        //Mask to int
        uint32_t ipv4_mask_int = 0;
        i = 0;
        s = mask_str;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            ipv4_mask_int += std::stoi(token) * std::pow(256,i++);
            s.erase(0, pos + delimiter.length());
        }
        ipv4_mask_int += std::stoi(token) * std::pow(256,i++);

        addr = ipv4_ip_int;
        netmask = ipv4_mask_int;
    }
    // this is IPv4 only
    std::string to_cidr() {
        std::string cidr = "";
        // I could write this as a loop but I think this is clearer
        uint8_t quad = (addr & 0xFF000000) >> 24;
        cidr.push_back(std::to_string(quad) + ".");
        uint8_t quad = (addr & 0x00FF0000) >> 16;
        cidr.push_back(std::to_string(quad) + ".");
        uint8_t quad = (addr & 0x0000FF00) >> 8;
        cidr.push_back(std::to_string(quad) + ".");
        uint8_t quad = (addr & 0x000000FF) >> 0;
        cidr.push_back(std::to_string(quad));
        cidr.push_back("/");
        // assume valid cidr netmask, e.g. no ones after the first zero
        uint8_t sz = 0;
        for (int i = 0; i < 32; i++) {
            if (netmask & (1 << i)) {
                sz++;
            }
        }
        cidr.push_back(std::to_string(sz));
        return cidr;
    }
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

