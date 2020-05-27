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

#ifndef TABLENAMES_H
#define TABLENAMES_H

#define RESULTS_TABLE "extrapolation_results"
#define DEPREF_RESULTS_TABLE "extrapolation_deprefer_results"
#define INVERSE_RESULTS_TABLE "extrapolation_inverse_results"
#define PEERS_TABLE "peers"
#define CUSTOMER_PROVIDER_TABLE "provider_customers"
#define ROAS_TABLE "roas"
#define STUBS_TABLE "stubs"
#define NON_STUBS_TABLE "non_stubs"
#define SUPERNODES_TABLE "supernodes"
#define ANNOUNCEMENTS_TABLE "mrt_w_roas"

// ROV++ Tables
#define POLICY_TABLE "rovpp_ases"
#define TOP_TABLE "rovpp_top_100_ases"
#define ETC_TABLE "rovpp_etc_ases"
#define EDGE_TABLE "rovpp_edge_ases"
#define VICTIM_TABLE "victims"
#define ATTACKER_TABLE "attackers"
#define ROVPP_BLACKHOLES_TABLE "rovpp_blackholes"

#endif
