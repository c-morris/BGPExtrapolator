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

#ifndef ANNOUNCEMENT_H
#define ANNOUNCEMENT_H

#include <cstdint>
#include <iostream>
#include <vector>

#include "Prefix.h"

class Announcement {
public:
    Prefix<> prefix;            // encoded with subnet mask
    uint32_t origin;            // origin ASN
    uint32_t priority;          // priority assigned based upon path
    uint32_t received_from_asn; // ASN that sent the ann
    bool from_monitor = false;  // flag for seeded ann
    int64_t tstamp;             // timestamp from mrt file
    uint32_t alt;               // flag for preventive alt ASN
    // TODO replace with proper templating
    uint32_t policy_index;      // stores the policy index the ann applies
    uint32_t tiebreak_override; // ensure tiebreaks propagate where they should

    /** Default constructor
     */
    Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t from_asn, int64_t timestamp = 0) {
        prefix.addr = aprefix;
        prefix.netmask = anetmask;
        origin = aorigin;
        received_from_asn = from_asn;
        priority = 0;
        from_monitor = false;
        tstamp = timestamp;
        policy_index = 0;
        alt = 0;
        tiebreak_override = 0;
    }
    
    /** Priority constructor
     */
    Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor = false) 
        : Announcement(aorigin, aprefix, anetmask, from_asn, timestamp) { 
        priority = pr; 
        from_monitor = a_from_monitor;
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
        os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << ',' << alt << '\n';
        return os;
    }
    
    bool operator==(const Announcement &b) const {
        return (origin == b.origin) &&
               (prefix == b.prefix) &&
               (priority == b.priority) &&
               (received_from_asn == b.received_from_asn);
    }
};
#endif
