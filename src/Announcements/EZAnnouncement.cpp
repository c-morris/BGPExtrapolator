#include "Announcements/EZAnnouncement.h"

/** Default constructor
 */
EZAnnouncement::EZAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
    uint32_t from_asn, int64_t timestamp /* = 0 */, bool from_attacker /* = false */) : Announcement(aorigin, aprefix, anetmask, from_asn, timestamp) {

    this->from_attacker = from_attacker;
}

/** Priority constructor
 */
EZAnnouncement::EZAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
    uint32_t pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor /* = false */, bool from_attacker /* = false */) : Announcement(aorigin, aprefix, anetmask, pr, from_asn, timestamp, a_from_monitor) {

    this->from_attacker = from_attacker;
}

/** Copy constructor
 */
EZAnnouncement::EZAnnouncement(const EZAnnouncement& ann) : Announcement(ann) {
    this->from_attacker = ann.from_attacker;
    this->as_path = ann.as_path;
}