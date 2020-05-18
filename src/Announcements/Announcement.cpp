#include "Announcements/Announcement.h"

Announcement::Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
    uint32_t from_asn, int64_t timestamp /* = 0 */) {
    
    prefix.addr = aprefix;
    prefix.netmask = anetmask;
    origin = aorigin;
    received_from_asn = from_asn;
    priority = 0;
    from_monitor = false;
    tstamp = timestamp;
}

Announcement::Announcement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
    uint32_t pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor /* = false */) 
    : Announcement(aorigin, aprefix, anetmask, from_asn, timestamp) {
    
    priority = pr; 
    from_monitor = a_from_monitor;
}

Announcement::Announcement(const Announcement& ann) {
    prefix = ann.prefix;           
    origin = ann.origin;           
    priority = ann.priority;         
    received_from_asn = ann.received_from_asn;
    from_monitor = ann.from_monitor; 
    tstamp = ann.tstamp;            
    policy_index = ann.policy_index;     
}

std::ostream& operator<<(std::ostream &os, const Announcement& ann) {
    os << "Prefix:\t\t" << std::hex << ann.prefix.addr << " & " << std::hex << 
        ann.prefix.netmask << std::endl << "Origin:\t\t" << std::dec << ann.origin
        << std::endl << "Priority:\t" << ann.priority << std::endl 
        << "Recv'd from:\t" << std::dec << ann.received_from_asn;
    return os;
}

std::ostream& Announcement::to_csv(std::ostream &os) {
    os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << '\n';
    return os;
}