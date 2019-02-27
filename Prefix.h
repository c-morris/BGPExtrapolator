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
        //i is 4 for ipv4
        int i = 3;
        std::string &s = addr_str;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            try {
              ipv4_ip_int += std::stoi(token) * std::pow(256,i--);
              s.erase(0, pos + delimiter.length());
            } catch(const std::out_of_range& e) {
              std::cerr << "Caught out of range error in Prefix constructor loop, token was: " << token << std::endl; 
            }
        }
        try {
          ipv4_ip_int += std::stoi(s) * std::pow(256,i--);
        } catch(const std::out_of_range& e) {
          std::cerr << "Caught out of range error in Prefix constructor, token was: " << token << std::endl; 
        }

        //Mask to int
        uint32_t ipv4_mask_int = 0;
        i = 3;
        s = mask_str;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            try {
              ipv4_mask_int += std::stoi(token) * std::pow(256,i--);
              s.erase(0, pos + delimiter.length());
            } catch(const std::out_of_range& e) {
              std::cerr << "Caught out of range error in Prefix constructor mask loop, token was: " << token << std::endl; 
            }
        }
        try {
          ipv4_mask_int += std::stoi(s) * std::pow(256,i--);
        } catch(const std::out_of_range& e) {
          std::cerr << "Caught out of range error in Prefix constructor mask, token was: " << token << std::endl; 
        }

        addr = ipv4_ip_int;
        netmask = ipv4_mask_int;
    }
    // this is IPv4 only
    std::string to_cidr() {
        std::string cidr = "";
        // I could write this as a loop but I think this is clearer
        uint8_t quad = (addr & 0xFF000000) >> 24;
        cidr.append(std::to_string(quad) + ".");
        quad = (addr & 0x00FF0000) >> 16;
        cidr.append(std::to_string(quad) + ".");
        quad = (addr & 0x0000FF00) >> 8;
        cidr.append(std::to_string(quad) + ".");
        quad = (addr & 0x000000FF) >> 0;
        cidr.append(std::to_string(quad));
        cidr.push_back('/');
        // assume valid cidr netmask, e.g. no ones after the first zero
        uint8_t sz = 0;
        for (int i = 0; i < 32; i++) {
            if (netmask & (1 << i)) {
                sz++;
            }
        }
        cidr.append(std::to_string(sz));
        return cidr;
    }
    // comparison operators for maps
    // comparing the addr first ensures the more specific address is greater
    bool operator<(const Prefix &b) const {
        uint64_t combined = 0;
        combined |= addr;
        combined = combined << 32;
        combined |= netmask; 
        uint64_t combined_b = 0;
        combined_b |= b.addr;
        combined_b = combined_b << 32;
        combined_b |= b.netmask; 
        return combined < combined_b;
    }
    bool operator==(const Prefix &b) const {
        return !(*this < b || b < *this);
    }
    bool operator>(const Prefix &b) const {
        return !(*this < b || *this == b);
    }
};

#endif

