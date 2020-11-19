#include "SQLQueriers/SQLQuerier.h"

/** Units tests for the SQLQuerier.cpp
 */

bool test_read_config_bgpdb() {
    try {
        SQLQuerier *querier = new SQLQuerier("announcement_table", "results_table", "inverse_results_table", "depref_results_table", "bgp", "/etc/bgp/bgp-test.conf", false);
    }
    catch (const std::exception &e) {
        std::cerr << "Config parsing failed (section bgp)" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}