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

#ifndef RANGRAPH_H
#define RANGRAPH_H

#include <stdlib.h>
#include <iostream>
#include <time.h>

#include "ASes/AS.h"
#include "Graphs/ASGraph.h"

ASGraph<>* ran_graph(int, int);
bool cyclic_util(ASGraph<>*, int, std::map<uint32_t, bool>*, std::map<uint32_t, bool>*);
bool is_cyclic(ASGraph<>*);

#endif
