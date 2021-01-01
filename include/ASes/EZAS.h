#ifndef EZ_AS_H
#define EZ_AS_H

#include "ASes/BaseAS.h"
#include "Announcements/EZAnnouncement.h"

#define EZAS_TYPE_BGP 0                               // Regular BGP
#define EZAS_TYPE_DIRECTORY_ONLY 64                   // No CD
#define EZAS_TYPE_COMMUNITY_DETECTION_LOCAL 65        // Local CD threshold Only
#define EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL 66       // Global CD Only (future work)
#define EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL_LOCAL 67 // Local and Global (future work)

class EZAS : public BaseAS<EZAnnouncement> {
public:
    bool attacker;
    bool adopter; // should we be using policy instead of this attribute?
    unsigned int policy;

    EZAS(uint32_t asn);
    EZAS();

    ~EZAS();

    virtual void process_announcement(EZAnnouncement &ann, bool ran=true);
};

#endif
