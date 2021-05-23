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

#include "Announcements/ROVppAnnouncement.h"

ROVppAnnouncement::ROVppAnnouncement() : Announcement() { }

ROVppAnnouncement::ROVppAnnouncement(uint32_t aorigin, 
                 Prefix<> prefix,
                 uint32_t from_asn, 
                 int64_t timestamp /* = 0 */) : Announcement<>(aorigin, prefix, from_asn, timestamp) {
    policy_index = 0;
    alt = 0;
    tiebreak_override = 0;
    sent_to_asn = 0;
    withdraw = false;
}

/** Priority constructor
 */
ROVppAnnouncement::ROVppAnnouncement(uint32_t aorigin, 
                                        Prefix<> prefix,
                                        uint32_t pr, 
                                        uint32_t from_asn, 
                                        int64_t timestamp, 
                                        const std::vector<uint32_t> &path,
                                        bool a_from_monitor /* = false */) 
    : ROVppAnnouncement(aorigin, prefix, from_asn, timestamp) {
    
    priority = pr; 
    from_monitor = a_from_monitor;
    as_path = path;
}

ROVppAnnouncement::ROVppAnnouncement(uint32_t aorigin, 
                                        Prefix<> prefix,
                                        uint32_t pr, 
                                        uint32_t from_asn, 
                                        int64_t timestamp, 
                                        uint32_t policy, 
                                        const std::vector<uint32_t> &path,
                                        bool a_from_monitor /* = false */) : ROVppAnnouncement(aorigin, prefix, pr, from_asn, timestamp, path, a_from_monitor) {
    
    policy_index = policy;
}

/** Copy constructor
 */
ROVppAnnouncement::ROVppAnnouncement(const ROVppAnnouncement& ann) : Announcement<>(ann) {
    alt = ann.alt;              
    policy_index = ann.policy_index;     
    tiebreak_override = ann.tiebreak_override;
    sent_to_asn = ann.sent_to_asn;       
    withdraw =  ann.withdraw;       
    priority = ann.priority;       
    // this is the important part
    as_path = ann.as_path; 
}

/** Copy assignment
 */
ROVppAnnouncement& ROVppAnnouncement::operator=(ROVppAnnouncement ann) {
    if(&ann == this)
        return *this;
    swap(*this, ann);
    return *this;
}

/** Swap
 */
void swap(ROVppAnnouncement& a, ROVppAnnouncement& b) {
    std::swap(a.prefix, b.prefix);
    std::swap(a.origin, b.origin);
    std::swap(a.priority, b.priority);
    std::swap(a.received_from_asn, b.received_from_asn);
    std::swap(a.from_monitor, b.from_monitor);
    std::swap(a.tstamp, b.tstamp);
    std::swap(a.alt, b.alt);
    std::swap(a.policy_index, b.policy_index);
    std::swap(a.tiebreak_override, b.tiebreak_override);
    std::swap(a.sent_to_asn, b.sent_to_asn);
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
std::ostream& operator<<(std::ostream &os, const ROVppAnnouncement& ann) {
    os << "Prefix:\t\t" << std::hex << ann.prefix.addr << " & " << std::hex << 
        ann.prefix.netmask << std::endl << "Origin:\t\t" << std::dec << ann.origin
        << std::endl << "Priority:\t" << ann.priority << std::endl 
        << "Recv'd from:\t" << std::dec << ann.received_from_asn << std::endl
        << "Sent to:\t" << std::dec << ann.sent_to_asn << std::endl
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
std::ostream& ROVppAnnouncement::to_csv(std::ostream &os) const {
    os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << ',' << alt << '\n';
    return os;
}

/** Passes the announcement struct data to an output stream to csv generation.
 * For creating the rovpp_blackholes table only.
 * 
 * @param &os Specifies the output stream.
 * @return The output stream parameter for reuse/recursion.
 */ 
std::ostream& ROVppAnnouncement::to_blackholes_csv(std::ostream &os) {
    os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << '\n';
    return os;
}

bool ROVppAnnouncement::operator==(const ROVppAnnouncement &b) const {
    return (origin == b.origin) &&
            (prefix == b.prefix) &&
            (as_path == b.as_path) &&
            (priority == b.priority) &&
            (sent_to_asn == b.sent_to_asn) &&
            (alt == b.alt) &&
            (received_from_asn == b.received_from_asn);
}

bool ROVppAnnouncement::operator!=(const ROVppAnnouncement &b) const {
    return !(*this == b);
}

bool ROVppAnnouncement::operator<(const ROVppAnnouncement &b) const {
    return (origin < b.origin) ||
            (prefix < b.prefix) ||
            (priority < b.priority) ||
            (sent_to_asn < b.sent_to_asn) ||
            (received_from_asn < b.received_from_asn) ||
            (alt < b.alt);
}