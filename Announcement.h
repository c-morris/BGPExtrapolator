#ifndef ANNOUNCEMENT_H
#define ANNOUNCEMENT_H

#include <cstdint>
#include <iostream>
#include "Prefix.h"

struct Announcement {
    Prefix<> prefix;            // encoded with subnet mask
    uint32_t origin;            // origin ASN
    double priority;            // priority assigned based upon path
    uint32_t received_from_asn; // ASN that sent the ann
    bool from_monitor = false;  // flag for seeded ann

    /** Default constructor
     */
    Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t from_asn) {
        prefix.addr = aprefix;
        prefix.netmask = anetmask;
        origin = aorigin;
        received_from_asn = from_asn;
        priority = 0.0;
        from_monitor = false;
    }
    
    /** Priority constructor
     */
    Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        double pr, uint32_t from_asn, bool a_from_monitor = false) 
        : Announcement(aorigin, aprefix, anetmask, from_asn) { 
        priority = pr; 
        from_monitor = a_from_monitor;
    }

    /** Comparison operators
     *  
     *  Comparing first on prefix ensures the most specific announcement gets priority. The origin is not used in comparison.
     *
     *  @ return true if operation holds, otherwise false
     */
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
    
    /** Converts the Announcement struct to a string for SQL queries.
     * 
     * @ return Announcement data in string form.
     */
    std::string to_sql(){
        return ("'" + prefix.to_cidr() + 
                "," + std::to_string(origin) + "'" + 
                "," + "'" + std::to_string(priority) + "'" + 
                "," + std::to_string(received_from_asn));
    }
    
    /** Defines the << operator for the Announcements
     *
     * For use in debugging, this operator prints an announcements to an output stream.
     * 
     * @ param &os Specifies the output stream.
     * @ param ann Specifies the announcement from which data is pulled.
     * @ return The output stream parameter for reuse/recursion.
     */ 
    friend std::ostream& operator<<(std::ostream &os, const Announcement& ann) {
        os << "Prefix:\t\t" << std::hex << ann.prefix.addr << " & " << std::hex << 
            ann.prefix.netmask << std::endl << "Origin:\t\t" << std::dec << ann.origin
            << std::endl << "Priority:\t" << ann.priority << std::endl 
            << "Recv'd from:\t" << std::dec << ann.received_from_asn;
        return os;
    }

    /** Passes the announcement struct data to an output stream to csv generation.
     *
     * @ param &os Specifies the output stream.
     * @ return The output stream parameter for reuse/recursion.
     */ 
    std::ostream& to_csv(std::ostream &os){
        os << prefix.to_cidr() << "," << origin << "," << received_from_asn << std::endl;
        return os;
    }
};

#endif
