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

#ifndef ROVPPEXTRAPOLATOR_H
#define ROVPPEXTRAPOLATOR_H

#include "Extrapolator.h"
#include "ROVppASGraph.h"

struct ROVppExtrapolator: public Extrapolator {
    ROVppExtrapolator(std::string r=RESULTS_TABLE,
                      std::string e=VICTIM_TABLE,
                      std::string f=ATTACKER_TABLE,
                      std::string g=POLICY_TABLE,
                      uint32_t iteration_size=false);

    ~ROVppExtrapolator();

    ////////////////////////////////////////////////////////////////////
    // Overidded Methods
    ////////////////////////////////////////////////////////////////////

    void perform_propagation(bool, size_t);
    void save_results(int iteration);
};

#endif
