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

#define ROA_VALID 0
#define ROA_UNKNOWN 1
#define ROA_INVALID_1 2
#define ROA_INVALID_2 3
#define ROA_INVALID_3 4 

class Announcement {
public:
    Prefix<> prefix;            // encoded with subnet mask
    uint32_t origin;            // origin ASN
    uint32_t priority;          // priority assigned based upon path
    uint32_t received_from_asn; // ASN that sent the ann
    bool from_monitor = false;  // flag for seeded ann
    int64_t tstamp;             // timestamp from mrt file
    uint32_t alt;               // flag meaning a "hole" along the path
    // TODO replace with proper templating
    uint32_t policy_index;      // stores the policy index the ann applies
    uint32_t tiebreak_override; // ensure tiebreaks propagate where they should
    bool withdraw;              // if this is a withdrawn route
    std::vector<uint32_t> as_path; // stores full as path
    uint32_t roa_validity;       // Inidicates the validity of the announcement (valid = 1; unknown = 2; invalid = 3; both = 4)

    /** Default constructor
     */
    Announcement(uint32_t aorigin, 
                 uint32_t aprefix, 
                 uint32_t anetmask,
                 uint32_t from_asn, 
                 int64_t timestamp = 0,
                 uint32_t roa_validity = 2) {
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
        withdraw = false;
        this->roa_validity = roa_validity;
    }
    
    /** Priority constructor
     */
    Announcement(uint32_t aorigin, 
                 uint32_t aprefix, 
                 uint32_t anetmask,
                 uint32_t pr, 
                 uint32_t from_asn, 
                 int64_t timestamp,
                 uint32_t roa_validity,
                 const std::vector<uint32_t> &path,
                 bool a_from_monitor = false) 
        : Announcement(aorigin, aprefix, anetmask, from_asn, timestamp, roa_validity) {
        priority = pr; 
        from_monitor = a_from_monitor;
        as_path = path;
    }

    /** Copy constructor
     */
    Announcement(const Announcement& ann) {
        prefix = ann.prefix;           
        origin = ann.origin;           
        priority = ann.priority;         
        received_from_asn = ann.received_from_asn;
        from_monitor = ann.from_monitor; 
        tstamp = ann.tstamp;
        roa_validity = ann.roa_validity;            
        alt = ann.alt;              
        policy_index = ann.policy_index;     
        tiebreak_override = ann.tiebreak_override;
        withdraw =  ann.withdraw;              
        // this is the important part
        as_path = ann.as_path; 
     }

    /** Copy assignment
     */
    Announcement& operator=(Announcement ann) {
        if(&ann == this)
            return *this;
        swap(*this, ann);
        return *this;
    }

    /** Swap
     */
    friend void swap(Announcement& a, Announcement& b) {
        std::swap(a.prefix, b.prefix);
        std::swap(a.origin, b.origin);
        std::swap(a.priority, b.priority);
        std::swap(a.received_from_asn, b.received_from_asn);
        std::swap(a.from_monitor, b.from_monitor);
        std::swap(a.tstamp, b.tstamp);
        std::swap(a.roa_validity, b.roa_validity);
        std::swap(a.alt, b.alt);
        std::swap(a.policy_index, b.policy_index);
        std::swap(a.tiebreak_override, b.tiebreak_override);
        std::swap(a.withdraw, b.withdraw);
        a.as_path.resize(b.as_path.size());
        std::swap(a.as_path, b.as_path);
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
            << "ROA Validity:\t" << ann.roa_validity << std::endl
            << "Recv'd from:\t" << std::dec << ann.received_from_asn << std::endl
            << std::endl
            << "Alt:\t\t" << std::dec << ann.alt << std::endl
            << "TieBrk:\t\t" << std::dec << ann.tiebreak_override << std::endl
            << "From Monitor:\t" << std::boolalpha << ann.from_monitor << std::endl
            << "Withdraw:\t" << std::boolalpha << ann.withdraw << std::endl
            << "AS_PATH\t";
            for (auto i : ann.as_path) { os << i << ' '; }
            os << std::endl;
        return os;
    }

    /** Passes the announcement struct data to an output stream to csv generation.
     *
     * @param &os Specifies the output stream.
     * @return The output stream parameter for reuse/recursion.
     */ 
    virtual std::ostream& to_csv(std::ostream &os){
        os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << ',' << alt << ',' << roa_validity << '\n';
        return os;
    }

    /** Passes the announcement struct data to an output stream to csv generation.
     * For creating the rovpp_blackholes table only.
     * 
     * @param &os Specifies the output stream.
     * @return The output stream parameter for reuse/recursion.
     */ 
    virtual std::ostream& to_blackholes_csv(std::ostream &os) {
        os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << ',' << roa_validity << '\n';
        return os;
    }
    
    bool operator==(const Announcement &b) const {
        return (origin == b.origin) &&
               (prefix == b.prefix) &&
               (as_path == b.as_path) &&
               (priority == b.priority) &&
               (alt == b.alt) &&
               (received_from_asn == b.received_from_asn) &&
               (roa_validity == b.roa_validity);
    }
    
    bool operator!=(const Announcement &b) const {
        return !(*this == b);
    }

    bool operator<(const Announcement &b) const {
        return (origin < b.origin) ||
               (prefix < b.prefix) ||
               (priority < b.priority) ||
               (received_from_asn < b.received_from_asn) ||
               (alt < b.alt);
    }
};
#endif
