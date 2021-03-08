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

#include "Logger.h"

typedef unsigned __int128 uint128_t;

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
    Prefix(Integer addr_in, Integer mask_in) {
        addr = addr_in;
        netmask = mask_in;
    }

    Prefix(const Prefix &p2) {
        this->addr = p2.addr;
        this->netmask = p2.netmask;
    }
        
    /** Priority constructor
     *
     * Takes a ipv4 address as input and converts it into two integers
     *
     * @param addr_str The IP address as a string.
     * @param mask_str The subnet mask/length as a string.
     */ 
    Prefix(std::string addr_str, std::string mask_str) {
        if (std::is_same<Integer, uint128_t>::value) {
            // IPv6 Address Parsing
            addr = ipv6_to_int(addr_str);  
            netmask = ipv6_to_int(mask_str);  
        } else {
            // IPv4 Address Parsing
            addr = ipv4_to_int(addr_str);  
            netmask = ipv4_to_int(mask_str);  
        }
    }
    
    /** Converts an IPv4 address or netmask as a string into a integer.
     *
     *  This is IPv4 only.
     *
     *  @return ipv4_ip_int An integer representation of an address / mask
     */
    uint32_t ipv4_to_int(std::string addr_str) const {
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
            BOOST_LOG_TRIVIAL(error) << "Caught malformed IPv4 address: " << addr_str;
        }
        return ipv4_ip_int;
    }
    
    
    /** Converts an IPv6 address or netmask as a string into a integer.
     *
     *  This is IPv6 only.
     *
     *  @return ipv6_ip_int An integer representation of an address / mask
     */
    uint128_t ipv6_to_int(std::string addr_str) const {
        int counter = 0;                // Check for proper addr length
        int omit_zeros_pos = -1;        // Stores the position of ommited blocks of zeros
        size_t pos = 0;                 // Position in string addr
        uint128_t ipv6_ip_int = 0;       // Stores the address as an int
        std::string token;              // Buffer subseciton of addr
        std::string delimiter = ":";    // String dilimiter
        bool error_f = false;           // Error flag, drops malformed input

        // Create a copy of address string
        std::string s = addr_str;
        if (s.empty()) {
            error_f = true;
        }
        else {
            while ((pos = s.find(delimiter)) != std::string::npos) {
                // Catch long malformed input
                if (counter > 7) {
                    error_f = true;
                    break;
                }
                // Token is one 4-byte int
                token = s.substr(0, pos);
                try {
                    // Encountered suppressed blocks of zeros
                    if (token.size() == 0) {
                        // Save the position
                        omit_zeros_pos = counter;
                        // Erase next semicolon
                        s.erase(0, pos + delimiter.length());
                        continue;
                    }
                    uint32_t token_int = std::stoi(token, 0, 16);

                    // Catch out of range ints, default to 65535
                    if (token_int > 65535) {
                        error_f = true;
                        break;
                    }
                    // Add token and shift left 16 bits
                    ipv6_ip_int += token_int;
                    ipv6_ip_int = ipv6_ip_int << 16;
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
            if (counter > 7) {
                error_f = true;
            } 

            try {
                // Add the last 16-bit token
                if (s.size() != 0) {
                    uint32_t s_int = std::stoi(s, 0, 16);
                    if (s_int > 65536) {
                        error_f = true;
                    }
                    else {
                        ipv6_ip_int += s_int;
                    }
                }

                // Insert ommited blocks of zeros
                if (omit_zeros_pos != -1) {
                    // Calculate the number of ommited blocks
                    int omit_blocks = 7 - counter;
                    // Split ipv6_ip_int into two separate numbers (split at the position of omitted blocks)
                    int shift_amount = (16 * (8 - (omit_blocks + omit_zeros_pos)));
                    // Shift left half right and move it back to clear the right half
                    uint128_t left_half = ipv6_ip_int >> shift_amount;
                    left_half = left_half << (16 * (8 - (omit_blocks + omit_zeros_pos)));
                    uint128_t right_half = left_half ^ ipv6_ip_int;
                    // Shift left half to add zeros in place of omitted blocks
                    left_half = left_half << (omit_blocks * 16);
                    // Reconstruct the address from the two halves
                    ipv6_ip_int = left_half | right_half;
                }
            } catch (...) {
                error_f = true;
            }
        }
        // Default errors to ::
        if (error_f == true) {
            ipv6_ip_int = 0;
            std::cout << "Caught malformed IPv6 address: " << addr_str << std::endl;
        }
        return ipv6_ip_int;
    }


    /** Converts this prefix into a cidr formatted string.
     *
     *  This is IPv4 only.
     *
     *  @return cidr A string in cidr format.
     */
    std::string to_cidr() const {
        // Check if this prefix is IPv6 or IPv4
        if (std::is_same<Integer, uint128_t>::value) {
            std::ostringstream cidr;
            for (int i = 112; i >= 0; i = i - 16) {
                // Process each quad separately
                int quad = (addr >> i) & 0xFFFF;
                // Convert quad to hex and add to cidr
                cidr << std::hex << quad;
                // Don't add a colon after the last quad
                if (i != 0) {
                    cidr << ":";
                }
            }
            cidr << '/';
            
            // Calculate the number of 1s in netmask
            // Not sure if that is what we are looking for
            uint32_t sz = 0;
            for (int i = 0; i < 128; i++) {
                if (netmask & (1 << i)) {
                    sz++;
                }
            }
            // Switch stringstream back to decimal
            cidr << std::dec << sz;
            return cidr.str();
        } else {
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
        return "";
    }
    
    /** operator<< is not defined for 128 bit integers
     *  Those two functions convert the address and netmask to strings
     *  They should only be used for debugging since they are pretty slow
     */
    std::string addr_to_string() const {
        std::string str;
        uint128_t num = addr;
        do {
            int digit = num % 10;
            str = std::to_string(digit) + str;
            num /= 10;
        } while (num != 0);
        return str;
    }
    std::string netmask_to_string() const {
        std::string str;
        uint128_t num = netmask;
        do {
            int digit = num % 10;
            str = std::to_string(digit) + str;
            num /= 10;
        } while (num != 0);
        return str;
    }

    /** Defined comparison operators for maps
     *
     * Comparing the addr first ensures the more specific address is greater
     *
     * @param b The Prefix object to which this object is compared.
     * @return true If the operation holds, otherwise false
     */
    bool operator<(const Prefix<Integer> &b) const {
        if (std::is_same<Integer, uint128_t>::value) {
            if (addr != b.addr) {
                return addr < b.addr;
            } else {
                return netmask < b.netmask;
            }
        } else {
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
    }
    bool operator==(const Prefix<Integer> &b) const {
        return !(*this < b || b < *this);
    }
    bool operator>(const Prefix<Integer> &b) const {
        return !(*this < b || *this == b);
    }
    bool operator!=(const Prefix<Integer> &b) const {
        return !(*this == b);
    }

    /** Check if this prefix is a subprefix of another, or is equal to it.
     *
     * @param b The other Prefix.
     * @return true if this prefix is contained in or equal to the other, else false.
     */
    bool contained_in_or_equal_to(const Prefix<Integer> &b) const {
        return b.netmask <= netmask && (addr & b.netmask) == (b.addr & b.netmask);
    }
};
#endif
