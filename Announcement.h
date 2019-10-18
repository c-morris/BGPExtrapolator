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

#include "Prefix.h"

class Announcement {
public:
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
    std::ostream& to_csv(std::ostream &os){
        os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << '\n';//std::endl;
        return os;
    }
};
#endif
