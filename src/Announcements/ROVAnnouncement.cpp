#include "Announcements/ROVAnnouncement.h"

ROVAnnouncement::ROVAnnouncement() : Announcement() { }

ROVAnnouncement::ROVAnnouncement(uint32_t aorigin, Prefix<> prefix, uint32_t from_asn, int64_t timestamp /* = 0 */) : 
    Announcement(aorigin, prefix, from_asn, timestamp) {}

ROVAnnouncement::ROVAnnouncement(uint32_t aorigin, Prefix<> prefix, Priority pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor /* = false */) : 
    Announcement(aorigin, prefix, pr, from_asn, timestamp, a_from_monitor) {}

ROVAnnouncement::ROVAnnouncement(const ROVAnnouncement& ann) : Announcement(ann) {}
