#include "ASes/EZAS.h"

EZAS::EZAS(uint32_t asn, uint32_t max_block_prefix_id) : BaseAS<EZAnnouncement>(asn, max_block_prefix_id, false, NULL) { }
EZAS::EZAS() : EZAS(0, 20) { }
EZAS::~EZAS() { }

void EZAS::process_announcement(EZAnnouncement &ann, bool ran) {
    //Paths with attackers are the only paths that need to be recorded
    if(ann.from_attacker) {
        //Don't accept if already on the path
        if(std::find(ann.as_path.begin(), ann.as_path.end(), asn) != ann.as_path.end())
            return;

        ann.as_path.insert(ann.as_path.begin(), asn);
    }

    BaseAS::process_announcement(ann, ran);
}