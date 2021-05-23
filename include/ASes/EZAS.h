#ifndef EZ_AS_H
#define EZ_AS_H

#include "ASes/BaseAS.h"
#include "Announcements/EZAnnouncement.h"

class EZAS : public BaseAS<EZAnnouncement> {
public:
    EZAS(uint32_t asn, uint32_t max_block_prefix_id);
    EZAS();

    ~EZAS();

    virtual void process_announcement(EZAnnouncement &ann, bool ran=true);
};

#endif