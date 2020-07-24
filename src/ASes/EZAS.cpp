#include "ASes/EZAS.h"

EZAS::EZAS(uint32_t myasn /* = 0 */, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results /* = NULL */) 
        : BaseAS<EZAnnouncement>(myasn, inverse_results) {
}

EZAS::~EZAS() {

}