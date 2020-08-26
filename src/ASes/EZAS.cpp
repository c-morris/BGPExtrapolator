#include "ASes/EZAS.h"

EZAS::EZAS(uint32_t asn) : BaseAS<EZAnnouncement>(asn, false, NULL) { }
EZAS::EZAS() : EZAS(0) { }
EZAS::~EZAS() { }

void EZAS::process_announcement(EZAnnouncement &ann, bool ran) {
    //Don't accept if already on the path
    if(std::find(ann.as_path.begin(), ann.as_path.end(), asn) != ann.as_path.end())
        return;

    ann.as_path.insert(ann.as_path.begin(), asn);
    BaseAS::process_announcement(ann, ran);
}