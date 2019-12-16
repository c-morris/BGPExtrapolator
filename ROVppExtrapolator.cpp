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

#include "ROVppExtrapolator.h"
#include "ROVppASGraph.h"

ROVppExtrapolator::ROVppExtrapolator(std::string r,
                           std::string e,
                           std::string f,
                           std::string g,
                           uint32_t iteration_size)
    : Extrapolator(new ROVppASGraph(), new ROVppSQLQuerier(e, f, g), iteration_size) {
    // Replace the ASGraph and SQLQuerier
    // ROVpp specific functions should use the rovpp_graph variable
    // The graph variable maintains backwards compatibility
}

ROVppExtrapolator::~ROVppExtrapolator() {}
