#ifndef ROV_ANNOUNCEMENT_H
#define ROV_ANNOUNCEMENT_H

#include "Announcement.h"

#include <cstdint>

class RovAnnouncement : public Announcement {
public:
    uint32_t rovData;

    RovAnnouncement(uint32_t origin, uint32_t rovData);
};

#endif