#include "ASes/EZAS.h"

EZAS::EZAS(uint32_t asn, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results) 
        : BaseAS<EZAnnouncement>(asn, inverse_results) {
}

EZAS::EZAS(uint32_t asn) : EZAS(asn, NULL) { }
EZAS::EZAS() : EZAS(0, NULL) { }

EZAS::~EZAS() {

}