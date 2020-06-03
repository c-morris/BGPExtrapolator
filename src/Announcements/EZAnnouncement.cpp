#include "Announcements/EZAnnouncement.h"

/** Default constructor
 */
EZAnnouncement::EZAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
    uint32_t from_asn, int64_t timestamp /* = 0 */) : Announcement(aorigin, aprefix, anetmask, from_asn, timestamp) {

}

/** Priority constructor
 */
EZAnnouncement::EZAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
    uint32_t pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor /* = false */) : Announcement(aorigin, aprefix, anetmask, pr, from_asn, timestamp, a_from_monitor) {

}

/** Copy constructor
 */
EZAnnouncement::EZAnnouncement(const EZAnnouncement& ann) : Announcement(ann) {

}