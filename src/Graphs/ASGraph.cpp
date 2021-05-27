/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#include "Graphs/ASGraph.h"

template <typename PrefixType>
ASGraph<PrefixType>::ASGraph(bool store_inverse_results, bool store_depref_results) : BaseGraph<AS<PrefixType>, PrefixType>(store_inverse_results, store_depref_results) {

}

template <typename PrefixType>
ASGraph<PrefixType>::~ASGraph() {

}

template <typename PrefixType>
AS<PrefixType>* ASGraph<PrefixType>::createNew(uint32_t asn) {
    return new AS<PrefixType>(asn, this->store_depref_results, this->inverse_results);
}

template class ASGraph<>;
template class ASGraph<uint128_t>;