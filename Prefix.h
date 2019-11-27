/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#ifndef PREFIX_H
#define PREFIX_H

#include <cmath>
#include <cstdint>
#include <string>
#include <iostream>


// Use uint32_t for IPv4, unsigned __int128 for IPv6
template <typename Integer = uint32_t>
class Prefix {
public:
    Integer addr;
    Integer netmask;
    
    /** Default constructor
     */
    Prefix() {}
   
    /** Integer input constructor
     */
    Prefix(uint32_t addr_in, uint32_t mask_in) {
        addr = addr_in;
        netmask = mask_in;
    }
        
    /** Priority constructor
     *
     * Takes a ipv4 address as input and converts it into two integers
     *
     * @param addr_str The IP address as a string.
     * @param mask_str The subnet mask/length as a string.
     */ 
    Prefix(std::string addr_str, std::string mask_str) {
        // IPv4 Address Parsing
        addr = addr_to_int(addr_str);  
        netmask = mask_to_int(mask_str);  
    }
    
    
    /** Converts a IPv4 address as a string into a integer.
     *
     *  This is IPv4 only.
     *
     *  @return ipv4_ip_int An integer representation of an address
     */
    uint32_t addr_to_int(std::string addr_str) const {
        int counter = 0;                // Check for proper addr length
        size_t pos = 0;                 // Position in string addr
        uint32_t ipv4_ip_int = 0;       // Stores the address as an int
        std::string token;              // Buffer subseciton of addr
        std::string delimiter = ".";    // String dilimiter
        bool error_f = false;           // Error flag, drops malformed input
       
        // Create a copy of address string
        std::string s = addr_str;
        if (s.empty()) {
            error_f = true;
        } else {
            while ((pos = s.find(delimiter)) != std::string::npos) {
                // Catch long malformed input
                if (counter > 3) {
                    error_f = true;
                    break;
                }
                // Token is one 8-bit int
                token = s.substr(0, pos);
                try {
                    uint32_t token_int = std::stoul(token); 
                    // Catch out of range ints, default to 255
                    if (token_int > 255) {
                        error_f = true;
                        break;
                    }
                    // Add token and shift left 8 bits
                    ipv4_ip_int += token_int;
                    ipv4_ip_int = ipv4_ip_int << 8;
                    // Trim token from addr
                    s.erase(0, pos + delimiter.length());
                    counter += 1;
                } catch (...) {
                    error_f = true;
                    break;
                }
            }
            // Catch short malformed input
            if (counter != 3) {
                error_f = true;
            }
            // Add last 8-bit token
            try {
                uint32_t s_int = std::stoul(s);
                if (s_int > 255) {
                    error_f = true;
                } else {
                    ipv4_ip_int += s_int;
                }
            } catch(...) {
                error_f = true;
            }
        }
        // Default errors to 0.0.0.0
        if (error_f == true) {
            ipv4_ip_int = 0;
            std::cerr << "Caught malformed IPv4 address: " << addr_str << std::endl;
        }
        return ipv4_ip_int;
    }
    
    
    /** Converts a IPv4 netmask as a string into a integer.
     *
     *  This is IPv4 only.
     *
     *  @return ipv4_mask_int An integer representation of a netmask
     */
    uint32_t mask_to_int(std::string mask_str) const {
        int counter = 0;                // Check for proper addr length
        size_t pos = 0;                 // Position in string addr
        uint32_t ipv4_mask_int = 0;     // Stores the netmask as an int
        std::string token;              // Buffer subseciton of addr
        std::string delimiter = ".";    // String dilimiter
        bool error_f = false;           // Error flag, drops malformed input

        std::string s = mask_str;
        if (s.empty()) {
            error_f = true;
        } else {
            while ((pos = s.find(delimiter)) != std::string::npos) { 
                // Catch long malformed input
                if (counter > 3) {
                    error_f = true;
                    break;
                }
                // Token is one 8-bit int
                token = s.substr(0, pos);
                try {
                    uint32_t token_int = std::stoul(token);
                    // Catch out of range ints, default to 255
                    if (token_int > 255) {
                        error_f = true;
                        break;
                    }
                    // Add token and shift left 8 bits
                    ipv4_mask_int += std::stoul(token);
                    ipv4_mask_int = ipv4_mask_int << 8;
                    // Trim token from addr
                    s.erase(0, pos + delimiter.length());
                    counter += 1;
                }
                catch (...) {
                    error_f = true;
                    break;
                }
            }
            // Catch short malformed input
            if (counter != 3) {
                error_f = true;
            }
            // Add last 8-bit token
            try {
                uint32_t s_int = std::stoul(s);
                if (s_int > 255) {
                    error_f = true;
                } else {
                    ipv4_mask_int += s_int;
                }
            }
            catch (...) {
                error_f = true;
            }
        }
        // Default errors to /0
        if (error_f == true) {
            ipv4_mask_int = 0;
            std::cerr << "Caught malformed IPv4 subnet mask: " << mask_str << std::endl;
        }
        return ipv4_mask_int;
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
     * @return true If the operation holds, otherwise false
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
    bool operator!=(const Prefix &b) const {
        return !(*this == b);
    }
};
#endif
