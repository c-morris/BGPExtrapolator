#include "SQLQueriers/SQLQuerier.h"

/** Units tests for the SQLQuerier.cpp
 */

// Create mock config file
bool test_querier_buildup() {
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
bool test_querier_teardown() {
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
        SQLQuerier<> *querier = new SQLQuerier<>("announcement_table", "results_table", "inverse_results_table", "depref_results_table", "full_path_results_table", -1, "test", "bgp-test.conf", false);

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

// Test for copy_to_db_string function
bool test_copy_to_db_string() {
    SQLQuerier<> *querier = new SQLQuerier<>("announcement_table", "results_table", "inverse_results_table", "depref_results_table", "full_path_results_table", -1, "test", "bgp-test.conf", false);

    if (querier->copy_to_db_query_string("test", "stubs", "(stub_asn,parent_asn)") != 
                                    "COPY stubs(stub_asn,parent_asn) FROM 'test' WITH (FORMAT csv)") {
        std::cerr << "test_copy_to_db failed" << std::endl;
        return false;
    }

    return true;
}

// Test for select_prefix_string function
bool test_select_prefix_string() {
    SQLQuerier<> *querier = new SQLQuerier<>("announcement_table", "results_table", "inverse_results_table", "depref_results_table", "full_path_results_table", -1, "test", "bgp-test.conf", false);
    Prefix<> *p = new Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);

    std::string true_results [4] = {
        "SELECT COUNT(*) FROM announcement_table WHERE prefix = '137.99.0.0/16';",
        "SELECT host(prefix), netmask(prefix), as_path, origin, time FROM announcement_table WHERE prefix = '137.99.0.0/16';",
        "SELECT COUNT(*) FROM announcement_table WHERE prefix <<= '137.99.0.0/16';",
        "SELECT host(prefix), netmask(prefix), as_path, origin, time FROM announcement_table WHERE prefix <<= '137.99.0.0/16';"
    };

    if (querier->select_prefix_query_string(p) != true_results[0]) {
        std::cerr << "test_select_prefix failed (query from select_prefix_count)" << std::endl;
        return false;
    }

    if (querier->select_prefix_query_string(p, false, "host(prefix), netmask(prefix), as_path, origin, time") != true_results[1]) {
        std::cerr << "test_select_prefix failed (query from select_prefix_ann)" << std::endl;
        return false;
    }

    if (querier->select_prefix_query_string(p, true) != true_results[2]) {
        std::cerr << "test_select_prefix failed (query from select_subnet_count)" << std::endl;
        return false;
    }

    if (querier->select_prefix_query_string(p, true, "host(prefix), netmask(prefix), as_path, origin, time") != true_results[3]) {
        std::cerr << "test_select_prefix failed (query from select_subnet_ann)" << std::endl;
        return false;
    }

    return true;
}

// Test for clear_table_string
bool test_clear_table_string() {
    SQLQuerier<> *querier = new SQLQuerier<>("announcement_table", "results_table", "inverse_results_table", "depref_results_table", "full_path_results_table", -1, "test", "bgp-test.conf", false);

    if (querier->clear_table_query_string("test_table") != "DROP TABLE IF EXISTS test_table;") {
        std::cerr << "test_clear_table_string failed" << std::endl;
        return false;
    }

    return true;
}

// Test for create_table_string
bool test_create_table_string() {
    SQLQuerier<> *querier = new SQLQuerier<>("announcement_table", "results_table", "inverse_results_table", "depref_results_table", "full_path_results_table", -1, "test", "bgp-test.conf", false);

    std::string true_results [3] = {
        "CREATE TABLE IF NOT EXISTS test_table (stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint);",
        "CREATE UNLOGGED TABLE IF NOT EXISTS test_table (stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint);",
        "CREATE UNLOGGED TABLE IF NOT EXISTS test_table (stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint); GRANT ALL ON TABLE test_table TO test_user;"
    };

    std::string sql = querier->create_table_query_string("test_table", "(stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint)");
    if (sql != true_results[0]) {
        std::cerr << "test_create_table_string failed" << std::endl;
        return false;
    }

    sql = querier->create_table_query_string("test_table", "(stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint)", true);
    if (sql != true_results[1]) {
        std::cerr << "test_create_table_string failed" << std::endl;
        return false;
    }

    sql = querier->create_table_query_string("test_table", "(stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint)", true, "test_user");
    if (sql != true_results[2]) {
        std::cerr << "test_create_table_string failed" << std::endl;
        return false;
    }

    return true;
}

// Test for select_max_query_string
bool test_select_max_query_string() {
    SQLQuerier<> *querier = new SQLQuerier<>("announcement_table", "results_table", "inverse_results_table", "depref_results_table", "full_path_results_table", -1, "test", "bgp-test.conf", false);

    std::string true_results [3] = {
        "SELECT MAX(block_id) FROM mrt_w_metadata;",
        "SELECT MAX(block_prefix_id) FROM mrt_w_metadata;",
        "SELECT MAX(block_prefix_id) FROM mrt_w_metadata_small;"
    };

    std::string sql = querier->select_max_query_string("mrt_w_metadata", "block_id");
    if (sql != true_results[0]) {
        std::cerr << "test_select_max_query_string failed" << std::endl;
        return false;
    }

    sql = querier->select_max_query_string("mrt_w_metadata", "block_prefix_id");
    if (sql != true_results[1]) {
        std::cerr << "test_select_max_query_string failed" << std::endl;
        return false;
    }

    sql = querier->select_max_query_string("mrt_w_metadata_small", "block_prefix_id");
    if (sql != true_results[2]) {
        std::cerr << "test_select_max_query_string failed" << std::endl;
        return false;
    }

    return true;
}