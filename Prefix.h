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

