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


// Announcement.h
BOOST_AUTO_TEST_CASE( Announcement_constructor ) {
        BOOST_CHECK( test_announcement() );
}
BOOST_AUTO_TEST_CASE( Ann_less_than_constructor ) {
        BOOST_CHECK( test_ann_lt_operator() );
}

// AS.cpp
BOOST_AUTO_TEST_CASE( AS_add_neighbor ) {
        BOOST_CHECK( test_add_neighbor() );
}
BOOST_AUTO_TEST_CASE( AS_receive_announcement ) {
        BOOST_CHECK( test_receive_announcement() );
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
BOOST_AUTO_TEST_CASE( ASGraph_decide_ranks ) {
        BOOST_CHECK( test_decide_ranks() );
}

// Extrapolator.cpp
BOOST_AUTO_TEST_CASE( Extrapolator_constructor ) {
        BOOST_CHECK( test_Extrapolator_constructor() );
}
BOOST_AUTO_TEST_CASE( Extrapolator_propagate_up ) {
        BOOST_CHECK( test_propagate_up() );
}
BOOST_AUTO_TEST_CASE( Extrapolator_propagate_down ) {
        BOOST_CHECK( test_propagate_down() );
}
BOOST_AUTO_TEST_CASE( Extrapolator_give_ann_to_as_path ) {
        BOOST_CHECK( test_give_ann_to_as_path() );
}
BOOST_AUTO_TEST_CASE( Extrapolator_send_all_announcements ) {
        BOOST_CHECK( test_send_all_announcements() );
}

#endif // RUN_TESTS


