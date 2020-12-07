#include "ASes/AS.h"

AS::AS(uint32_t asn, bool store_depref_results, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results) : BaseAS(asn, store_depref_results, inverse_results) { }
AS::AS(uint32_t asn, bool store_depref_results) : AS(asn, store_depref_results, NULL) { }
AS::AS(uint32_t asn) : AS(asn, false, NULL) { }
AS::AS() : AS(0, false, NULL) { }

AS::~AS() { }

template<>
std::ostream& operator<< <Announcement>(std::ostream &os, const BaseAS<Announcement>& as) {
    os << "ASN: " << as.asn << std::endl << "Rank: " << as.rank
        << std::endl << "Providers: ";
    for (auto &provider : *as.providers) {
        os << provider << ' ';
    }
    os << '\n';
    os << "Peers: ";
    for (auto &peer : *as.peers) {
        os << peer << ' ';
    }
    os << '\n';
    os << "Customers: ";
    for (auto &customer : *as.customers) {
        os << customer << ' ';
    }
    os << '\n';
    return os;
}


