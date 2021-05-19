#include "Announcements/ROVAnnouncement.h"

ROVAnnouncement::ROVAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
    uint32_t from_asn, int64_t timestamp /* = 0 */) : Announcement(aorigin, aprefix, anetmask, from_asn, timestamp) {}

ROVAnnouncement::ROVAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
    Priority pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor /* = false */, uint32_t prefix_id /* = 0 */) : 
    Announcement(aorigin, aprefix, anetmask, pr, from_asn, timestamp, a_from_monitor, prefix_id) {}

ROVAnnouncement::ROVAnnouncement(const ROVAnnouncement& ann) : Announcement(ann) {}