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

#include "Announcements/Announcement.h"

template <typename PrefixType>
Announcement<PrefixType>::Announcement() : prefix(0, 0, 0, 0) {
    tstamp = -1;
}

template <typename PrefixType>
Announcement<PrefixType>::Announcement(uint32_t aorigin, Prefix<PrefixType> prefix,
    uint32_t from_asn, int64_t timestamp /* = 0 */) : prefix(prefix) {
    
    //prefix = prefix;
    origin = aorigin;
    received_from_asn = from_asn;
    from_monitor = false;
    tstamp = timestamp;
}

template <typename PrefixType>
Announcement<PrefixType>::Announcement(uint32_t aorigin, Prefix<PrefixType> prefix,
    Priority pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor /* = false */) 
    : Announcement(aorigin, prefix, from_asn, timestamp) {
    
    priority = pr; 
    from_monitor = a_from_monitor;
}

template <typename PrefixType>
Announcement<PrefixType>::Announcement(const Announcement<PrefixType>& ann) {
    prefix = ann.prefix;           
    origin = ann.origin;           
    priority = ann.priority;         
    received_from_asn = ann.received_from_asn;
    from_monitor = ann.from_monitor; 
    tstamp = ann.tstamp;            
    policy_index = ann.policy_index;  
}

//****************** FILE I/O ******************//
template <typename PrefixType>
std::ostream& operator<<(std::ostream &os, const Announcement<PrefixType>& ann) {
    os << "Prefix:\t\t" << std::hex << ann.prefix.addr_to_string() << " & " << std::hex << 
        ann.prefix.netmask_to_string() << std::endl << "Prefix_ID:\t\t" << ann.prefix.id << std::endl
        << "Origin:\t\t" << std::dec << ann.origin << std::endl << "Priority:\t" << ann.priority << std::endl 
        << "Recv'd from:\t" << std::dec << ann.received_from_asn;
    return os;
}
template <typename PrefixType>
std::ostream& Announcement<PrefixType>::to_csv(std::ostream &os) const {
    os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << ',' << prefix.id << '\n';
    return os;
}

template class Announcement<>;
template class Announcement<uint128_t>;
