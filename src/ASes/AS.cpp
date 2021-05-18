#include "ASes/AS.h"

template <typename PrefixType>
AS<PrefixType>::AS(uint32_t asn, uint32_t max_block_prefix_id, bool store_depref_results, std::map<std::pair<Prefix<PrefixType>, uint32_t>, std::set<uint32_t>*> *inverse_results) 
    : BaseAS<Announcement<PrefixType>, PrefixType>(asn, max_block_prefix_id, store_depref_results, inverse_results) { }

template <typename PrefixType>
AS<PrefixType>::AS(uint32_t asn, uint32_t max_block_prefix_id, bool store_depref_results) : AS<PrefixType>(asn, max_block_prefix_id, store_depref_results, NULL) { }

template <typename PrefixType>
AS<PrefixType>::AS(uint32_t asn, uint32_t max_block_prefix_id) : AS<PrefixType>(asn, max_block_prefix_id, false, NULL) { }

template <typename PrefixType>
AS<PrefixType>::AS() : AS<PrefixType>(0, 20, false, NULL) { }

template <typename PrefixType>
AS<PrefixType>::~AS() { }

template class AS<>;
template class AS<uint128_t>;
