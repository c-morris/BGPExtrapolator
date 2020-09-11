#ifndef EZ_AS_H
#define EZ_AS_H

#include "ASes/BaseAS.h"
#include "Announcements/EZAnnouncement.h"

class EZAS : public BaseAS<EZAnnouncement> {
public:
    bool attacker;
    bool adopter;

    EZAS(uint32_t asn);
    EZAS();

    ~EZAS();

    virtual void process_announcement(EZAnnouncement &ann, bool ran=true);
};

#endif