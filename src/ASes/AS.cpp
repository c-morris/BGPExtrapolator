#include "ASes/AS.h"

AS::AS(uint32_t asn, bool store_depref_results, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results) : BaseAS(asn, store_depref_results, inverse_results) { }
AS::AS(uint32_t asn, bool store_depref_results) : AS(asn, store_depref_results, NULL) { }
AS::AS(uint32_t asn) : AS(asn, false, NULL) { }
AS::AS() : AS(0, false, NULL) { }

AS::~AS() { }