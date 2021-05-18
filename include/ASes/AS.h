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

#ifndef AS_H
#define AS_H

#include <type_traits>

#include "ASes/BaseAS.h"
#include "Announcements/Announcement.h"

template <typename PrefixType = uint32_t>
class AS : public BaseAS<Announcement<PrefixType>, PrefixType> {
public:
    AS(uint32_t asn, uint32_t max_block_prefix_id, bool store_depref_results, std::map<std::pair<Prefix<PrefixType>, uint32_t>, std::set<uint32_t>*> *inverse_results);
    AS(uint32_t asn, uint32_t max_block_prefix_id, bool store_depref_results);
    AS(uint32_t asn, uint32_t max_block_prefix_id = 20);
    AS();

    ~AS();
};

#endif