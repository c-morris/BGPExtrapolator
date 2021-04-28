#ifndef ROV_ANNOUNCEMENT_H
#define ROV_ANNOUNCEMENT_H

#include "Announcements/Announcement.h"

class ROVAnnouncement : public Announcement {
public:
    /** Default constructor
     */
    ROVAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t from_asn, int64_t timestamp = 0);
    
    /** Priority constructor
     */
    ROVAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        Priority pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor = false);

    /** Copy constructor
     */
    ROVAnnouncement(const ROVAnnouncement& ann);
};

#endif