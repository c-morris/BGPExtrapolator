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

#include <vector>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <vector>

#include "Prefix.h"

struct Announcement {
    Prefix<> prefix;                // encoded with subnet mask
    uint32_t origin;                // origin ASN
    float priority;                 // priority assigned based upon path
    uint32_t received_from_asn;     // ASN that sent the ann
    uint32_t inference_l;           // stores the path's inference length
    bool from_monitor = false;      // flag for seeded ann
    int64_t tstamp;                 // timestamp from mrt file
    std::vector<uint32_t> as_path;  // stores full as path
    std::vector<bool> tb_flag;      // flags each hop as a tie

    /** Default constructor
     */
    Announcement(uint32_t aorigin, 
                 uint32_t aprefix, 
                 uint32_t anetmask,
                 uint32_t from_asn
                 int64_t timestamp = 0) {
        prefix.addr = aprefix;
        prefix.netmask = anetmask;
        origin = aorigin;
        received_from_asn = from_asn;
        priority = 0.0;
        inference_l = 0;
        from_monitor = false;
        tstamp = timestamp;
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
                 int64_t timestamp,
                 bool a_from_monitor = false)
                 : Announcement(aorigin, 
                                aprefix, 
                                anetmask, 
                                from_asn,
                                timestamp) { 
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
        // AS Path is stored in reverse, output starts at end of vector
        for (auto it = as_path.rbegin(); it != as_path.rend(); ++it) {
            if (it != as_path.rbegin()) { os << ','; }
            os << *it;
        }   
        os << "}\"," << tstamp << ',' << inference_l << '\n';
        return os; 
        return os;
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
