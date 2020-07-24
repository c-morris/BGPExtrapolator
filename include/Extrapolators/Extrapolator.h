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

class Extrapolator : public BlockedExtrapolator<SQLQuerier, ASGraph, Announcement, AS> {
public:
    Extrapolator(bool random_tiebraking = true,
                    bool store_invert_results = true, 
                    bool store_depref_results = false, 
                    std::string announcement_table = ANNOUNCEMENTS_TABLE,
                    std::string results_table = RESULTS_TABLE, 
                    std::string inverse_results_table = INVERSE_RESULTS_TABLE, 
                    std::string depref_results_table = DEPREF_RESULTS_TABLE, 
                    uint32_t iteration_size = false);

    ~Extrapolator();

};

#endif