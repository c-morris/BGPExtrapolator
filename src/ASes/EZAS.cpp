#include "ASes/EZAS.h"

EZAS::EZAS(uint32_t myasn /* = 0 */, 
        std::set<uint32_t> *attackers /* = NULL */,
        std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results /* = NULL */,
        std::set<uint32_t> *prov /* = NULL */, 
        std::set<uint32_t> *peer /* = NULL */,
        std::set<uint32_t> *cust /* = NULL */) : BaseAS<EZAnnouncement>(myasn, inverse_results, prov, peer, cust) {

}

EZAS::~EZAS() {

}