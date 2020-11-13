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

#include "Extrapolators/Extrapolator.h"

Extrapolator::Extrapolator(bool random_tiebraking,
                            bool store_invert_results, 
                            bool store_depref_results, 
                            std::string announcement_table,
                            std::string results_table, 
                            std::string inverse_results_table, 
                            std::string depref_results_table,
                            std::string config_section, 
                            uint32_t iteration_size) : BlockedExtrapolator(random_tiebraking, store_invert_results, store_depref_results, iteration_size) {

    graph = new ASGraph(store_invert_results, store_depref_results);
    querier = new SQLQuerier(announcement_table, results_table, inverse_results_table, depref_results_table, config_section);
}

Extrapolator::Extrapolator() : Extrapolator(DEFAULT_RANDOM_TIEBRAKING, DEFAULT_STORE_INVERT_RESULTS, DEFAULT_STORE_DEPREF_RESULTS, 
                                            ANNOUNCEMENTS_TABLE, RESULTS_TABLE, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, "bgp", DEFAULT_ITERATION_SIZE) { }

Extrapolator::~Extrapolator() { }