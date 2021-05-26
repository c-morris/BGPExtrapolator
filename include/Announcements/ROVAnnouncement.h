#ifndef ROV_ANNOUNCEMENT_H
#define ROV_ANNOUNCEMENT_H

#include "Announcements/Announcement.h"

class ROVAnnouncement : public Announcement<> {
public:
    /** "Uninitialized" constructor
     */
    ROVAnnouncement();

    /** Default constructor
     */
    ROVAnnouncement(uint32_t aorigin, Prefix<> prefix,
        uint32_t from_asn, int64_t timestamp = 0);
    
    /** Priority constructor
     */
    ROVAnnouncement(uint32_t aorigin, Prefix<> prefix,
        Priority pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor = false);

    /** Copy constructor
     */
    ROVAnnouncement(const ROVAnnouncement& ann);
};

#endif
