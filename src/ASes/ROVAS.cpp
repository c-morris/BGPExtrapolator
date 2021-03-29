#include "ASes/ROVAS.h"

ROVAS::ROVAS(uint32_t asn, std::set<uint32_t> *rov_attackers, bool store_depref_results, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results) 
: BaseAS<ROVAnnouncement>(asn, store_depref_results, inverse_results) { 
    // Save reference to attackers
    attackers = rov_attackers;
}
ROVAS::ROVAS(uint32_t asn, std::set<uint32_t> *rov_attackers, bool store_depref_results) 
: BaseAS<ROVAnnouncement>(asn, store_depref_results, NULL) { 
    // Save reference to attackers
    attackers = rov_attackers;
}
ROVAS::ROVAS(uint32_t asn, std::set<uint32_t> *rov_attackers)
: BaseAS<ROVAnnouncement>(asn, false, NULL) { 
    // Save reference to attackers
    attackers = rov_attackers;
}
ROVAS::ROVAS() : ROVAS(0, NULL, false, NULL) { }
ROVAS::~ROVAS() { }

void ROVAS::process_announcement(ROVAnnouncement &ann, bool ran) {
    std::cout << "processing ann" << std::endl;
    std::cout << "asn: " << this->asn << ", adopts: " << adopts_rov << std::endl;

    if (!adopts_rov) {
        // If this AS doesn't adopt ROV, process the announcement
        std::cout << "PROCESSED ann, doesn't adopt" << std::endl;
        BaseAS::process_announcement(ann, ran);
    } else if (is_attacker()) {
        // If this AS is an attacker, process the announcement
        std::cout << "PROCESSED ann, attacker" << std::endl;
        BaseAS::process_announcement(ann, ran);
    } else if (adopts_rov && !is_from_attacker(ann)) {
        // If this AS adopts ROV, but the announcement is not from an attacker, process it
        std::cout << "PROCESSED ann, not from attacker" << std::endl;
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