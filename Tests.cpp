#ifdef RUN_TESTS
#include "Tests.h"
#define BOOST_TEST_MODULE ExtrapolatorTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

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
BOOST_AUTO_TEST_CASE( ASGraph_add_relationship ) {
        BOOST_CHECK( test_add_relationship() );
}
#endif // RUN_TESTS


