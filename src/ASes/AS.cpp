#include "ASes/AS.h"

AS::AS(uint32_t asn, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results) : BaseAS(asn, inverse_results) { }
AS::AS(uint32_t asn) : AS(asn, NULL) { }
AS::AS() : AS(0, NULL) { }

AS::~AS() { }