#include "ASes/EZAS.h"
#include "CommunityDetection.h"

EZAS::EZAS(CommunityDetection *community_detection, uint32_t asn) : BaseAS<EZAnnouncement>(asn, false, NULL), community_detection(community_detection) {

}

EZAS::EZAS(uint32_t asn) : EZAS(NULL, asn) {

}

EZAS::EZAS() : EZAS(0) { }
EZAS::~EZAS() { }

void EZAS::process_announcement(EZAnnouncement &ann, bool ran) {
    //Don't accept if already on the path
    if(std::find(ann.as_path.begin(), ann.as_path.end(), asn) != ann.as_path.end())
        return;

    if (community_detection != NULL && (policy == EZAS_TYPE_DIRECTORY_ONLY || policy == EZAS_TYPE_COMMUNITY_DETECTION_LOCAL)) {
        for(uint32_t asn : ann.as_path) {
            if(community_detection->blacklist_asns.find(asn) != community_detection->blacklist_asns.end())
                return;
        }

        //TODO This could use some improvement
        for(auto &blacklisted_path : community_detection->blacklist_paths) {
            if(std::includes(ann.as_path.begin(), ann.as_path.end(), blacklisted_path.begin(), blacklisted_path.end()))
                return;
        }
    }

    ann.as_path.insert(ann.as_path.begin(), asn);

    BaseAS::process_announcement(ann, ran);
}
