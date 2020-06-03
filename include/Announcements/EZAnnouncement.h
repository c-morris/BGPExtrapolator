#ifndef EZ_ANNOUNCEMENT_H
#define EZ_ANNOUNCEMENT_H

#include "Announcements/Announcement.h"

class EZAnnouncement : public Announcement {
public:
    /** Default constructor
     */
    EZAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t from_asn, int64_t timestamp = 0);
    
    /** Priority constructor
     */
    EZAnnouncement(uint32_t aorigin, uint32_t aprefix, uint32_t anetmask,
        uint32_t pr, uint32_t from_asn, int64_t timestamp, bool a_from_monitor = false);

    /** Copy constructor
     */
    EZAnnouncement(const EZAnnouncement& ann);
};

#endif