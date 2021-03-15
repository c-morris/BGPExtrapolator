#ifndef EZ_AS_H
#define EZ_AS_H

#include <vector>
#include <set>
#include <unordered_set>

#include "ASes/BaseAS.h"
#include "Announcements/EZAnnouncement.h"

#define EZAS_TYPE_BGP 0                               // Regular BGP
#define EZAS_TYPE_DIRECTORY_ONLY 64                   // No CD
#define EZAS_TYPE_COMMUNITY_DETECTION_LOCAL 65        // Local CD threshold Only
#define EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL 66       // Global CD Only (future work)
#define EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL_LOCAL 67 // Local and Global (future work)
#define EZAS_TYPE_BGPSEC 68
#define EZAS_TYPE_BGPSEC_TRANSITIVE 69

//Forward Declaration to deal with circular dependency
class CommunityDetection;

class EZAS : public BaseAS<EZAnnouncement> {
public:
    CommunityDetection *community_detection;
    std::set<std::vector<uint32_t>> blacklist_paths;
    std::unordered_set<uint32_t> blacklist_asns;
    unsigned int policy;

    EZAS(CommunityDetection *community_detection, uint32_t asn);
    EZAS(uint32_t asn);
    EZAS();

    ~EZAS();

    virtual void process_announcement(EZAnnouncement &ann, bool ran=true);
};
#endif
