#include "ASes/ROVAS.h"

ROVAS::ROVAS(uint32_t asn, std::set<uint32_t> *rov_attackers, bool store_depref_results, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results) 
: BaseAS<ROVAnnouncement>(asn, store_depref_results, inverse_results) { 
    // Save reference to attackers
    attackers = rov_attackers;
    // ROV adoption is false by default
    adopts_rov = false;
}
ROVAS::ROVAS(uint32_t asn, std::set<uint32_t> *rov_attackers, bool store_depref_results) : ROVAS(asn, rov_attackers, store_depref_results, NULL) {}
ROVAS::ROVAS(uint32_t asn, std::set<uint32_t> *rov_attackers) : ROVAS(asn, rov_attackers, false, NULL) {}
ROVAS::ROVAS(uint32_t asn) : ROVAS(asn, NULL, false, NULL) {}
ROVAS::ROVAS() : ROVAS(0, NULL, false, NULL) {}
ROVAS::~ROVAS() {}

void ROVAS::process_announcement(ROVAnnouncement &ann, bool ran) {
    if (!adopts_rov) {
        // If this AS doesn't adopt ROV, process the announcement
        BaseAS::process_announcement(ann, ran);
    } else if (is_attacker()) {
        // If this AS is an attacker, process the announcement
        BaseAS::process_announcement(ann, ran);
    } else if (adopts_rov && !is_from_attacker(ann)) {
        // If this AS adopts ROV, but the announcement is not from an attacker, process it
        BaseAS::process_announcement(ann, ran);
    }
}

bool ROVAS::is_from_attacker(ROVAnnouncement &ann) {
    if (attackers != NULL) {
        return !(attackers->find(ann.origin) == attackers->end());
    } else {
        return false;
    }
}

bool ROVAS::is_attacker() {
    if (attackers != NULL) {
        return !(attackers->find(this->asn) == attackers->end());
    } else {
        return false;
    }
}

void ROVAS::set_rov_adoption(bool adopts_rov) {
    this->adopts_rov = adopts_rov;
}

bool ROVAS::get_rov_adoption() {
    return adopts_rov;
}