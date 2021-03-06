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

#ifndef EXTRAPOLATOR_H
#define EXTRAPOLATOR_H

#include "Extrapolators/BlockedExtrapolator.h"

template <typename PrefixType = uint32_t>
class Extrapolator : public BlockedExtrapolator<SQLQuerier<PrefixType>, ASGraph<PrefixType>, Announcement<PrefixType>, AS<PrefixType>, PrefixType> {
public:
    Extrapolator(bool random_tiebraking,
                    bool store_results, 
                    bool store_invert_results, 
                    bool store_depref_results, 
                    std::string announcement_table,
                    std::string results_table, 
                    std::string inverse_results_table, 
                    std::string depref_results_table,
                    std::string full_path_results_table,
                    std::string config_section, 
                    uint32_t iteration_size,
                    int exclude_as_number,
                    uint32_t mh_mode,
                    bool origin_only,
                    std::vector<uint32_t> *full_path_asns,
                    int max_threads,
                    bool select_block_id);

    Extrapolator();
    ~Extrapolator();
};

#endif
