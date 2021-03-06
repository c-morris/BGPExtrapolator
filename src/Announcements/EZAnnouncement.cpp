#include "Announcements/EZAnnouncement.h"

EZAnnouncement::EZAnnouncement() : Announcement() { }

EZAnnouncement::EZAnnouncement(uint32_t aorigin, Prefix<> prefix, uint32_t from_asn, int64_t timestamp /* = 0 */, 
    bool from_attacker /* = false */) : Announcement<>(aorigin, prefix, from_asn, timestamp) {

    this->from_attacker = from_attacker;
}

EZAnnouncement::EZAnnouncement(uint32_t aorigin, Prefix<> prefix, Priority pr, uint32_t from_asn, int64_t timestamp, 
    bool a_from_monitor /* = false */, bool from_attacker /* = false */) : Announcement<>(aorigin, prefix, pr, from_asn, timestamp, a_from_monitor) {

    this->from_attacker = from_attacker;
}

EZAnnouncement::EZAnnouncement(const EZAnnouncement& ann) : Announcement<>(ann) {
    this->from_attacker = ann.from_attacker;
    this->as_path = ann.as_path;
}
