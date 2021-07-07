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

//Vanilla Tables
#define RESULTS_TABLE "extrapolation_results"
#define FULL_PATH_RESULTS_TABLE "full_path_extrapolation_results"
#define DEPREF_RESULTS_TABLE "extrapolation_deprefer_results"
#define INVERSE_RESULTS_TABLE "extrapolation_inverse_results"
#define PEERS_TABLE "peers"
#define CUSTOMER_PROVIDER_TABLE "provider_customers"
#define STUBS_TABLE "stubs"
#define NON_STUBS_TABLE "non_stubs"
#define SUPERNODES_TABLE "supernodes"
#define ANNOUNCEMENTS_TABLE "mrt_w_roas"

// Simulation Tables
#define SIMULATION_RESULTS_TABLE "simulation_extrapolation_results_raw_round_1"
#define ROV_ANNOUNCEMENTS_TABLE "mrt_w_metadata"

// ROV++ Tables
#define ROVPP_POLICY_TABLE "rovpp_ases"
#define ROVPP_TOP_TABLE "rovpp_top_100_ases"
#define ROVPP_ETC_TABLE "rovpp_etc_ases"
#define ROVPP_EDGE_TABLE "rovpp_edge_ases"
#define ROVPP_SIMULATION_TABLE "simulation_announcements"
#define ROVPP_BLACKHOLES_TABLE "rovpp_blackholes"
#define ROVPP_ROAS_TABLE "roas"

#define ROVPP_RESULTS_TABLE "rovpp_extrapolation_results"
#define ROVPP_PEERS_TABLE "peers"
#define ROVPP_CUSTOMER_PROVIDER_TABLE "provider_customers"
#define ROVPP_ANNOUNCEMENTS_TABLE "mrt_w_roas"
#define ROVPP_TRACKED_ASES_TABLE "tracked_ases"

//EzBGPsec Tables
#define EZBGPSEC_AS_CATAGORIES_TABLE "good_customer_pairs"
#define EZBGPSEC_ANNOUNCEMENTS_TABLE "good_customer_pairs_ann"
#define EZBGPSEC_ROUND_TABLE_BASE_NAME "simulation_extrapolation_results_raw_round_"

#endif
