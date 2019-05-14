#ifndef PREFIX_H
#define PREFIX_H

#include <cmath>
#include <cstdint>
#include <string>
#include <iostream>

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
        size_t pos = 0;                 // Position in string addr
        std::string token;              // Buffer subseciton of addr

        //IP to int
        bool error_f = false;           // Error flag, drops malformed input
        int counter = 0;                // Check for proper addr length
        uint32_t ipv4_ip_int = 0;
        
        // Convert string address to an unsigned 32-bit int
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
                // May need try-catch
                uint32_t token_int = std::stoul(token);
                
                // Catch out of range ints, default to 255
                if (token_int > 255) {
                    error_f = true;
                    break;
                }
                
                // Add token and shift left 8 bits
                ipv4_ip_int += std::stoul(token);
                ipv4_ip_int = ipv4_ip_int << 8;
                
                // Trim token from addr
                s.erase(0, pos + delimiter.length());
                counter += 1;
            }
            
            // Catch short malformed input
            if (counter != 3) {
                error_f = true;
            }
            
            // Add last 8-bit token
            // May need try-catch
            uint32_t s_int = std::stoul(s);
            if (s_int > 255) {
                error_f = true;
            } else {
                ipv4_ip_int += s_int;
            }
        }
        

        // Convert Subnet Mask to int
        counter = 0;                // Check for proper addr length
        uint32_t ipv4_mask_int = 0;
        
        s = mask_str;
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
                
            // Catch short malformed input
            if (counter != 3) {
                error_f = true;
            }
                
            // Add last 8-bit token
            uint32_t s_int = std::stoul(s);
            if (s_int > 255) {
                error_f = true;
            } else {
                ipv4_mask_int += s_int;
            }
        }
        
        // Default errors to 0.0.0.0/0
        if (error_f == true) {
            ipv4_ip_int = 0;
            ipv4_mask_int = 0;
            std::cerr << "Caught malformed IPv4 address: " << addr_str << std::endl;
            std::cerr << "Or IPv4 mask: " << mask_str << std::endl;
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
    bool operator!=(const Prefix &b) const {
        return !(*this == b);
    }
};

#endif

