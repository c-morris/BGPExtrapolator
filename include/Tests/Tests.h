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

#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <assert.h>
#include <random>
#include <map>
#include <typeinfo>
#include <cmath>    // Need?
#include <chrono>   // Need?

#include "SQLQueriers/SQLQuerier.h"
#include "ASes/AS.h"
#include "Graphs/ASGraph.h"
#include "Announcements/Announcement.h"
#include "Extrapolators/Extrapolator.h"

#include "SQLQueriers/ROVppSQLQuerier.h"
#include "ASes/ROVppAS.h"
#include "Graphs/ROVppASGraph.h"
#include "Announcements/ROVppAnnouncement.h"
#include "Extrapolators/ROVppExtrapolator.h"

#include "Prefix.h"
#include "Logger.h"

// Prototypes for PrefixTest.cpp
bool test_prefix();
bool test_string_to_cidr();
bool test_prefix_lt_operator();
bool test_prefix_gt_operator();
bool test_prefix_eq_operator();
bool test_prefix_contained_in_or_equal_to_operator();

// Prototypes for AnnouncementTest.cpp
bool test_announcement();
bool test_ann_lt_operator();
bool test_ann_gt_operator();
bool test_to_sql();
bool test_ann_os_operator();
bool test_to_csv();

// Prototypes for ASTest.cpp
bool test_get_random();
bool test_add_neighbor();
bool test_remove_neighbor();
bool test_receive_announcements();
bool test_process_announcement();
bool test_process_announcements();
bool test_already_received();
bool test_clear_announcements();

// Prototypes for ASGraphTest.cpp
bool test_add_relationship();
bool test_translate_asn();
bool test_decide_ranks();
bool test_remove_stubs();
bool test_tarjan();
bool test_combine_components();

// Prototypes for ExtrapolatorTest.cpp
bool test_Extrapolator_constructor();
bool test_propagate_up();
bool test_propagate_up_multihomed_standard();
bool test_propagate_up_multihomed_peer_mode();
bool test_propagate_down();
bool test_propagate_down2();
bool test_propagate_down_multihomed_standard();
bool test_save_results_parallel();
bool test_give_ann_to_as_path();
bool test_send_all_announcements();
bool test_send_all_announcements_multihomed_standard1();
bool test_send_all_announcements_multihomed_standard2();
bool test_send_all_announcements_multihomed_peer_mode1();
bool test_send_all_announcements_multihomed_peer_mode2();
bool test_extrapolate_blocks_buildup();
bool test_extrapolate_blocks_teardown();
bool test_extrapolate_blocks();

// Prototypes for ROVppTest.cpp
bool test_rovpp_ann_eq_operator();
bool test_ROVppExtrapolator_constructor();
bool test_rovpp_propagate_up();
bool test_rovpp_propagate_down();
bool test_rovpp_give_ann_to_as_path();
bool test_rovpp_send_all_announcements();
bool test_rovpp_add_relationship();
bool test_rovpp_translate_asn();
bool test_rovpp_decide_ranks();
bool test_rovpp_remove_stubs();
bool test_rovpp_combine_components();
bool test_rovpp_get_random();
bool test_rovpp_add_neighbor();
bool test_rovpp_remove_neighbor();
bool test_rovpp_receive_announcements();
bool test_rovpp_process_announcement();
bool test_rovpp_process_announcements();
bool test_rovpp_already_received();
bool test_rovpp_clear_announcements();
bool test_rovpp_announcement();
bool test_rovpp_pass_rov();
bool test_rovpp_rov_receive_announcements();
bool test_best_alternative_route();
bool test_best_alternative_route_chosen();
bool test_rovpp_tiebreak_override();
bool test_withdrawal();
bool test_tiny_hash();
bool test_rovpp_full_path();

//EZBGPsec
bool ezbgpsec_test_path_propagation();

//SQLQuerier
bool test_parse_config_buildup();
bool test_parse_config_teardown();
bool test_parse_config();

#endif
