#ifndef NANNOUNCEMENT_H
#define NANNOUNCEMENT_H

#include <cstdint>
#include <iostream>
#include <set>
#include "Prefix.h"
#include "Announcement.h"

/** Supports ROV++ negative announcements. 
 * 
 * An ROV++ negative announcement is like a normal BGP announcement but with
 * an additional subprefix which this AS has *no* valid route to. This feature
 * is designed to mitigate collateral damage associated with subprefix attacks
 * with ROV. 
 */
struct NegativeAnnouncement: public Announcement {
    std::set<Prefix<>> null_routed; // subprefixes without valid routes

    NegativeAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t from_asn, std::set<Prefix<>> n_routed) 
    : Announcement(aorigin, aprefix, anetmask, from_asn) {
        null_routed = n_routed;
    }
    
    /** Priority constructor
     */
    NegativeAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        double pr, uint32_t from_asn, bool a_from_monitor, std::set<Prefix<>> n_routed) 
    : Announcement(aorigin, aprefix, anetmask, pr, from_asn, false) { 
        null_routed = n_routed;
    }

    /** Add a subprefix to the set of null_routed subprefixes.
     */
    void null_route_subprefix(Prefix<> p) {
        // maybe add error checking to confirm p is a subprefix of this->prefix
        null_routed.insert(p);
    }

    /** Defines the << operator for the NegativeAnnouncements
     *
     * For use in debugging, this operator prints an announcements to an output stream.
     * 
     * @param &os Specifies the output stream.
     * @param ann Specifies the announcement from which data is pulled.
     * @return The output stream parameter for reuse/recursion.
     */ 
    friend std::ostream& operator<<(std::ostream &os, const NegativeAnnouncement& ann) {
        os << "Prefix:\t\t" << std::hex << ann.prefix.addr << " & " << std::hex << 
            ann.prefix.netmask << std::endl << "Origin:\t\t" << std::dec << ann.origin
            << std::endl << "Priority:\t" << ann.priority << std::endl 
            << "Recv'd from:\t" << std::dec << ann.received_from_asn << std::endl;
        os << "Null-routed subprefixes: ";
        for (auto p : ann.null_routed) {
            os << p.to_cidr() << " ";
        }
        os << std::endl;
        return os;
    }
};
#endif
