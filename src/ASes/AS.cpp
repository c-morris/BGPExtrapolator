#include "ASes/AS.h"

template <typename PrefixType>
AS<PrefixType>::AS(uint32_t asn, bool store_depref_results, std::map<std::pair<Prefix<PrefixType>, uint32_t>, std::set<uint32_t>*> *inverse_results) 
    : BaseAS<Announcement<PrefixType>, PrefixType>(asn, store_depref_results, inverse_results) { }

template <typename PrefixType>
AS<PrefixType>::AS(uint32_t asn, bool store_depref_results) : AS<PrefixType>(asn, store_depref_results, NULL) { }

template <typename PrefixType>
AS<PrefixType>::AS(uint32_t asn) : AS<PrefixType>(asn, false, NULL) { }

template <typename PrefixType>
AS<PrefixType>::AS() : AS<PrefixType>(0, false, NULL) { }

template <typename PrefixType>
AS<PrefixType>::~AS() { }

template class AS<>;
template class AS<uint128_t>;