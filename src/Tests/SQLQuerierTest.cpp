#include "SQLQueriers/SQLQuerier.h"

/** Units tests for the SQLQuerier.cpp
 */

// Create mock config file
bool test_parse_config_buildup() {
    try {
        std::ofstream config("bgp-test.conf");
        config << "[test]\nhost = test0\ndatabase = test1\npassword = test2\nuser = test3\nport = test4\nram = test\nrestart_postgres_cmd = test";
        config.close();
        return true;
    }
    catch (const std::exception &e) {
        std::cerr << "Failed to create config file" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }
}

//Delete mock config file
bool test_parse_config_teardown() {
    if (std::remove("bgp-test.conf") != 0) {
        std::cerr << "Failed to remove config file" << std::endl;
        return false;
    }
    return true;
}

// Test config parsing
bool test_parse_config() {
    bool failed = false;
    try {
        SQLQuerier *querier = new SQLQuerier("announcement_table", "results_table", "inverse_results_table", "depref_results_table", -1, "test", "bgp-test.conf", false);

        if (querier->host != "test0"){
            std::cerr << "Failed to parse password" << std::endl;
            failed = true;
        }
        if (querier->db_name != "test1") {
            std::cerr << "Failed to parse database name" << std::endl;
            failed = true;
        }
        if (querier->pass != "test2") {
            std::cerr << "Failed to parse password" << std::endl;
            failed = true;
        }
        if (querier->user != "test3") {
            std::cerr << "Failed to parse username" << std::endl;
            failed = true;
        }
        if (querier->port != "test4") {
            std::cerr << "Failed to parse port" << std::endl;
            failed = true;
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Config parsing failed (section bgp)" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    return !failed;
}