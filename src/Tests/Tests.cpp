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

#ifdef RUN_TESTS
#include "Tests/Tests.h"
#define BOOST_TEST_MODULE ExtrapolatorTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

struct Config {
        Config() {
                Logger::init_logger(true, "", 0);

                //Enable and disable in desired test functions
                boost::log::core::get()->set_logging_enabled(false);
        }
};

BOOST_GLOBAL_FIXTURE( Config );

// // Prefix.h
// BOOST_AUTO_TEST_CASE( Prefix_constructor ) {
//         BOOST_CHECK( test_prefix() );
// }
// BOOST_AUTO_TEST_CASE( Prefix_to_cidr ) {
//         BOOST_CHECK( test_string_to_cidr() );
// }
// BOOST_AUTO_TEST_CASE( Prefix_less_than_operator ) {
//         BOOST_CHECK( test_prefix_lt_operator() );
// }
// BOOST_AUTO_TEST_CASE( Prefix_greater_than_operator ) {
//         BOOST_CHECK( test_prefix_gt_operator() );
// }
// BOOST_AUTO_TEST_CASE( Prefix_equivalence_operator ) {
//         BOOST_CHECK( test_prefix_eq_operator() );
// }
// BOOST_AUTO_TEST_CASE( Prefix_contained_in_or_equal_to_operator ) {
//         BOOST_CHECK( test_prefix_contained_in_or_equal_to_operator() );
// }


// // Announcement.h
// BOOST_AUTO_TEST_CASE( Announcement_constructor ) {
//         BOOST_CHECK( test_announcement() );
// }

// // AS.cpp
// BOOST_AUTO_TEST_CASE( AS_get_random ) {
//         BOOST_CHECK( test_get_random() );
// }
// BOOST_AUTO_TEST_CASE( AS_add_neighbor ) {
//         BOOST_CHECK( test_add_neighbor() );
// }
// BOOST_AUTO_TEST_CASE( AS_remove_neighbor ) {
//         BOOST_CHECK( test_remove_neighbor() );
// }
// BOOST_AUTO_TEST_CASE( AS_receive_announcements ) {
//         BOOST_CHECK( test_receive_announcements() );
// }
// BOOST_AUTO_TEST_CASE( AS_process_announcement ) {
//         BOOST_CHECK( test_process_announcement() );
// }
// BOOST_AUTO_TEST_CASE( AS_process_announcements ) {
//         BOOST_CHECK( test_process_announcements() );
// }
// BOOST_AUTO_TEST_CASE( AS_already_received ) {
//         BOOST_CHECK( test_already_received() );
// }
// BOOST_AUTO_TEST_CASE( AS_clear_announcements ) {
//         BOOST_CHECK( test_clear_announcements() );
// }

// // ASGraph.cpp
// BOOST_AUTO_TEST_CASE( ASGraph_add_relationship ) {
//         BOOST_CHECK( test_add_relationship() );
// }
// BOOST_AUTO_TEST_CASE( ASGraph_translate_asn ) {
//         BOOST_CHECK( test_translate_asn() );
// }
// BOOST_AUTO_TEST_CASE( ASGraph_decide_ranks ) {
//         BOOST_CHECK( test_decide_ranks() );
// }
// BOOST_AUTO_TEST_CASE( ASGraph_tarjan_test ) {
//         BOOST_CHECK( test_tarjan() );
// }
// BOOST_AUTO_TEST_CASE( ASGraph_combine_components_test ) {
//         BOOST_CHECK( test_combine_components() );
// }

// // Extrapolator.cpp
// BOOST_AUTO_TEST_CASE( Extrapolator_constructor ) {
//         BOOST_CHECK( test_Extrapolator_constructor() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_give_ann_to_as_path ) {
//         BOOST_CHECK( test_give_ann_to_as_path() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_propagate_up ) {
//         BOOST_CHECK( test_propagate_up() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_propagate_up_multihomed_standard ) {
//         BOOST_CHECK( test_propagate_up_multihomed_standard() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_propagate_up_multihomed_peer_mode ) {
//         BOOST_CHECK( test_propagate_up_multihomed_peer_mode() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_propagate_down ) {
//         BOOST_CHECK( test_propagate_down() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_propagate_down2 ) {
//         BOOST_CHECK( test_propagate_down2() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_propagate_down_multihomed_standard ) {
//         BOOST_CHECK( test_propagate_down_multihomed_standard() );
// }
BOOST_AUTO_TEST_CASE( Extrapolator_save_results_parallel ) {
        BOOST_CHECK( test_save_results_parallel() );
}
// BOOST_AUTO_TEST_CASE( Extrapolator_send_all_announcements ) {
//         BOOST_CHECK( test_send_all_announcements() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_send_all_announcements_multihomed_standard1 ) {
//         BOOST_CHECK( test_send_all_announcements_multihomed_standard1() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_send_all_announcements_multihomed_standard2 ) {
//         BOOST_CHECK( test_send_all_announcements_multihomed_standard2() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_send_all_announcements_multihomed_peer_mode1 ) {
//         BOOST_CHECK( test_send_all_announcements_multihomed_peer_mode1() );
// }
// BOOST_AUTO_TEST_CASE( Extrapolator_send_all_announcements_multihomed_peer_mode2 ) {
//         BOOST_CHECK( test_send_all_announcements_multihomed_peer_mode2() );
// }
BOOST_AUTO_TEST_CASE( Extrapolator_test_extrapolate_blocks ) {
        BOOST_CHECK( test_extrapolate_blocks_buildup() );
        BOOST_CHECK( test_extrapolate_blocks() );
        //BOOST_CHECK( test_extrapolate_blocks_teardown() );
}

// // ROVpp
// BOOST_AUTO_TEST_CASE( Announcement_eqality_operator ) {
//         BOOST_CHECK( test_rovpp_ann_eq_operator() );
// }

// BOOST_AUTO_TEST_CASE( ROVppExtrapolator_constructor ) {
//         BOOST_CHECK( test_ROVppExtrapolator_constructor() );
// }
// BOOST_AUTO_TEST_CASE( ROVppExtrapolator_give_ann_to_as_path ) {
//         BOOST_CHECK( test_rovpp_give_ann_to_as_path() );
// }
// BOOST_AUTO_TEST_CASE( ROVppExtrapolator_propagate_up ) {
//         BOOST_CHECK( test_rovpp_propagate_up() );
// }
// BOOST_AUTO_TEST_CASE( ROVppExtrapolator_propagate_down ) {
//         BOOST_CHECK( test_rovpp_propagate_down() );
// }
// BOOST_AUTO_TEST_CASE( ROVppExtrapolator_send_all_announcements ) {
//         BOOST_CHECK( test_rovpp_send_all_announcements() );
// }
// BOOST_AUTO_TEST_CASE( ROVppASGraph_add_relationship ) {
//         BOOST_CHECK( test_rovpp_add_relationship() );
// }
// BOOST_AUTO_TEST_CASE( ROVppASGraph_translate_asn ) {
//         BOOST_CHECK( test_rovpp_translate_asn() );
// }
// BOOST_AUTO_TEST_CASE( ROVppASGraph_decide_ranks ) {
//         BOOST_CHECK( test_rovpp_decide_ranks() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_get_random ) {
//         BOOST_CHECK( test_rovpp_get_random() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_pass_rov ) {
//         BOOST_CHECK( test_rovpp_pass_rov() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_add_neighbor ) {
//         BOOST_CHECK( test_rovpp_add_neighbor() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_remove_neighbor ) {
//         BOOST_CHECK( test_rovpp_remove_neighbor() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_receive_announcements ) {
//         BOOST_CHECK( test_rovpp_receive_announcements() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_ROV_receive_announcements ) {
//         BOOST_CHECK( test_rovpp_rov_receive_announcements() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_process_announcement ) {
//         BOOST_CHECK( test_rovpp_process_announcement() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_process_announcements ) {
//         BOOST_CHECK( test_rovpp_process_announcements() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_already_received ) {
//         BOOST_CHECK( test_rovpp_already_received() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAS_clear_announcements ) {
//         BOOST_CHECK( test_rovpp_clear_announcements() );
// }
// BOOST_AUTO_TEST_CASE( ROVppAnnouncement_constructor ) {
//         BOOST_CHECK( test_rovpp_announcement() );
// }
// BOOST_AUTO_TEST_CASE( ROVpp_best_alternative_route ) {
//         BOOST_CHECK( test_best_alternative_route() );
// }
// // Uncomment this when the test is fixed. 
// //BOOST_AUTO_TEST_CASE( ROVpp_best_alternative_route_chosen ) {
// //        BOOST_CHECK( test_best_alternative_route_chosen() );
// //}
// BOOST_AUTO_TEST_CASE( ROVpp_tiebreak_override ) {
//         BOOST_CHECK( test_rovpp_tiebreak_override() );
// }
// BOOST_AUTO_TEST_CASE( ROVpp_test_withdrawal ) {
//         BOOST_CHECK( test_withdrawal() );
// }
// BOOST_AUTO_TEST_CASE( ROVpp_test_tiny_hash ) {
//         BOOST_CHECK( test_tiny_hash() );
// }
// BOOST_AUTO_TEST_CASE( ROVpp_test_full_path ) {
//         BOOST_CHECK( test_rovpp_full_path() );
// }

// //EZBGPsec Tests

// BOOST_AUTO_TEST_CASE( EZBGPsec_test_path_propagation ) {
//         BOOST_CHECK( ezbgpsec_test_path_propagation() );
// }

// //SQLQuerier Tests
// BOOST_AUTO_TEST_CASE( SQLQuerier_test_parse_config ) {
//         BOOST_CHECK ( test_parse_config_buildup() );
//         BOOST_CHECK( test_parse_config() );
//         BOOST_CHECK ( test_parse_config_teardown() );
// }

#endif // RUN_TESTS


