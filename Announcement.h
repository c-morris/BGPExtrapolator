#ifndef ANNOUNCEMENT_H
#define ANNOUNCEMENT_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <iterator>
#include "Prefix.h"

struct Announcement {
    Prefix<> prefix;            // encoded with subnet mask
    uint32_t origin;            // origin ASN
    float priority;             // priority assigned based upon path
    uint32_t received_from_asn; // ASN that sent the ann
    uint32_t inference_l;       // stores the path's inference length
    bool from_monitor = false;  // flag for seeded ann
    std::vector<uint32_t> as_path;

    /** Default constructor
     */
    Announcement(uint32_t aorigin, 
                 uint32_t aprefix, 
                 uint32_t anetmask,
                 uint32_t from_asn) {
        prefix.addr = aprefix;
        prefix.netmask = anetmask;
        origin = aorigin;
        received_from_asn = from_asn;
        priority = 0.0;
        inference_l = 0;
        from_monitor = false;
    }
    
    /** Priority constructor
     */
    Announcement(uint32_t aorigin, 
                 uint32_t aprefix, 
                 uint32_t anetmask, 
                 uint32_t from_asn, 
                 uint32_t length, 
                 double pr, 
                 const std::vector<uint32_t> &path,
                 bool a_from_monitor = false)
                 : Announcement(aorigin, 
                                aprefix, 
                                anetmask, 
                                from_asn) { 
        priority = pr;
        inference_l = length;
        from_monitor = a_from_monitor;
        as_path = path;
    }
    
    /** Defines the << operator for the Announcements
     *
     * For use in debugging, this operator prints an announcements to an output stream.
     * 
     * @param &os Specifies the output stream.
     * @param ann Specifies the announcement from which data is pulled.
     * @return The output stream parameter for reuse/recursion.
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
     * @param &os Specifies the output stream.
     * @return The output stream parameter for reuse/recursion.
     */ 
    virtual std::ostream& to_csv(std::ostream &os){
        os << prefix.to_cidr() << ',' << origin << ",\"{";
        for (std::vector<uint32_t>::iterator it = as_path.begin(); it != as_path.end(); ++it) {
            if (it != as_path.begin()) { os << ','; }
            os << *it;
        }   
        os << "}\"," << inference_l << '\n';
        return os; 
        /** OLD output
        os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << '\n';//std::endl;
        return os;
        */
    }
};

struct FPAnnouncement : public Announcement {
    
    FPAnnouncement(uint32_t aorigin, 
                   uint32_t aprefix, 
                   uint32_t anetmask, 
                   uint32_t from_asn, 
                   uint32_t length, 
                   double pr, 
                   const std::vector<uint32_t> &path,
                   bool a_from_monitor = false)
    : Announcement(aorigin, aprefix, anetmask, from_asn, length, pr, path, a_from_monitor) { }
    
    /** Passes the announcement struct data with path to an output stream for csv generation.
     *
     * @param &os Specifies the output stream.
     * @return The output stream parameter for reuse/recursion.
     */ 
    std::ostream& to_csv(std::ostream &os){
        os << prefix.to_cidr() << ',' << origin << ",\"{";
        for (auto asn : as_path) {
            os << asn << ",";
        }   
        os << "}\"," << inference_l << std::endl;
        return os; 
    }   
};

#endif
