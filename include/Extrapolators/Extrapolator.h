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
protected:
    std::vector<uint32_t> verify_ases = {0};
    
public:
    
    Extrapolator(bool random_tiebraking,
                    bool store_invert_results, 
                    bool store_depref_results, 
                    std::string announcement_table,
                    std::string results_table, 
                    std::string inverse_results_table, 
                    std::string depref_results_table,
                    std::string config_section, 
                    uint32_t iteration_size);
    
    Extrapolator(bool random_tiebraking,
                    bool store_invert_results, 
                    bool store_depref_results, 
                    std::string announcement_table,
                    std::string results_table, 
                    std::string inverse_results_table, 
                    std::string depref_results_table,
                    std::vector<uint32_t> verify_ases,
                    std::string config_section, 
                    uint32_t iteration_size);

    Extrapolator();
    ~Extrapolator();
    
};

#endif
