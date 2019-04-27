#ifndef PREFIX_H
#define PREFIX_H

#include <cmath>
#include <cstdint>
#include <string>

// use uint32_t for IPv4, unsigned __int128 for IPv6
template <typename Integer = uint32_t>
struct Prefix {
    Integer addr;
    Integer netmask;
    
    /** Default constructor
     */
    Prefix() {}
    
    /** Priority constructor
     *
     * NEEDS DESCRIPTION
     *
     * @param addr_str The IP address as a string.
     * @param mask_str The subnet mask/length as a string.
     */ 
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
    
    /** Converts this prefix into a cidr formatted string.
     *
     *  This is IPv4 only.
     *
     *  @return cidr A string in cidr format.
     */
    std::string to_cidr() const {
        std::string cidr = "";
        uint8_t quad = (addr & 0xFF000000) >> 24;
        cidr.append(std::to_string(quad) + ".");
        quad = (addr & 0x00FF0000) >> 16;
        cidr.append(std::to_string(quad) + ".");
        quad = (addr & 0x0000FF00) >> 8;
        cidr.append(std::to_string(quad) + ".");
        quad = (addr & 0x000000FF) >> 0;
        cidr.append(std::to_string(quad));
        cidr.push_back('/');
        // Assume valid cidr netmask, e.g. no ones after the first zero
        uint8_t sz = 0;
        for (int i = 0; i < 32; i++) {
            if (netmask & (1 << i)) {
                sz++;
            }
        }
        cidr.append(std::to_string(sz));
        return cidr;
    }
    
    /** Defined comparison operators for maps
     *
     * Comparing the addr first ensures the more specific address is greater
     *
     * @param b The Prefix object to which this object is compared.
     * @return true if the operation holds, otherwise false
     */
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

