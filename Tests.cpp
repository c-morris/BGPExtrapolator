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
#include "Tests.h"
#define BOOST_TEST_MODULE ExtrapolatorTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

// Prefix.h
BOOST_AUTO_TEST_CASE( Prefix_constructor ) {
        BOOST_CHECK( test_prefix() );
}
BOOST_AUTO_TEST_CASE( Prefix_to_cidr ) {
        BOOST_CHECK( test_string_to_cidr() );
}
BOOST_AUTO_TEST_CASE( Prefix_less_than_operator ) {
        BOOST_CHECK( test_prefix_lt_operator() );
}
BOOST_AUTO_TEST_CASE( Prefix_greater_than_operator ) {
        BOOST_CHECK( test_prefix_gt_operator() );
}
BOOST_AUTO_TEST_CASE( Prefix_equivalence_operator ) {
        BOOST_CHECK( test_prefix_eq_operator() );
}

// AS.cpp
BOOST_AUTO_TEST_CASE( AS_add_neighbor ) {
        BOOST_CHECK( test_add_neighbor() );
}
BOOST_AUTO_TEST_CASE( AS_process_announcement ) {
        BOOST_CHECK( test_process_announcement() );
}
BOOST_AUTO_TEST_CASE( AS_receive_announcements ) {
        BOOST_CHECK( test_receive_announcements() );
}
BOOST_AUTO_TEST_CASE( AS_already_received ) {
        BOOST_CHECK( test_already_received() );
}
BOOST_AUTO_TEST_CASE( AS_clear_announcements ) {
        BOOST_CHECK( test_clear_announcements() );
}
BOOST_AUTO_TEST_CASE( AS_process_announcements ) {
        BOOST_CHECK( test_process_announcements() );
}

// ASGraph.cpp
BOOST_AUTO_TEST_CASE( ASGraph_add_relationship ) {
        BOOST_CHECK( test_add_relationship() );
}
BOOST_AUTO_TEST_CASE( ASGraph_translate_asn ) {
        BOOST_CHECK( test_translate_asn() );
}
BOOST_AUTO_TEST_CASE( ASGraph_create_graph) {
        BOOST_CHECK( test_create_graph_from_db() );
}
BOOST_AUTO_TEST_CASE( ASGraph_decide_ranks ) {
        BOOST_CHECK( test_decide_ranks() );
}
BOOST_AUTO_TEST_CASE( ASGraph_tarjan_test ) {
        BOOST_CHECK( test_tarjan() );
}
BOOST_AUTO_TEST_CASE( ASGraph_combine_components_test ) {
        BOOST_CHECK( test_combine_components() );
}

// Extrapolator.cpp
BOOST_AUTO_TEST_CASE( Extrapolator_constructor ) {
        BOOST_CHECK( test_Extrapolator_constructor() );
}
BOOST_AUTO_TEST_CASE( Extrapolator_give_ann_to_as_path ) {
        BOOST_CHECK( test_give_ann_to_as_path() );
}
BOOST_AUTO_TEST_CASE( Extrapolator_propagate_up ) {
        BOOST_CHECK( test_propagate_up() );
}
BOOST_AUTO_TEST_CASE( Extrapolator_propagate_down ) {
        BOOST_CHECK( test_propagate_down() );
}
BOOST_AUTO_TEST_CASE( Extrapolator_send_all_announcements ) {
        BOOST_CHECK( test_send_all_announcements() );
}

#endif // RUN_TESTS


