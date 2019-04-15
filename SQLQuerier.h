#ifndef SQLQUERIER_H
#define SQLQUERIER_H

#define IPV4 4
#define IPV6 6

struct ASGraph;

#include <pqxx/pqxx>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "ASGraph.h"
#include "TableNames.h"
#include "TableNames.h"

struct SQLQuerier {
    SQLQuerier();
    ~SQLQuerier();

    void open_connection();
    void close_connection();
    pqxx::result execute(std::string sql, bool insert = false);
    pqxx::result select_from_table(std::string table_name, int limit = 0);
    pqxx::result select_ann_records(std::string table_name, std::string prefix = "", int limit = 0);
    pqxx::result select_ann_records(std::string table_name, std::vector<std::string> prefixes, int limit = 0);
    pqxx::result select_distinct_prefixes_from_table(std::string table_name);
    pqxx::result select_roa_prefixes(std::string table_name, int ip_family = IPV4);
    void check_for_relationship_changes(std::string peers_table_1,
                                                std::string customer_provider_pairs_table_1,
                                                std::string peers_table_2,
                                                std::string customer_provider_paris_table_2);
    void insert_results(ASGraph* graph, std::string results_table_name);
    void copy_results_to_db(std::string file_name);
    void clear_stubs_from_db();
    void copy_stubs_to_db(std::string file_name);
    void copy_supernodes_to_db(std::string file_name);
    void create_supernodes_tbl();
    void create_stubs_tbl();
    void create_results_tbl();
    void create_results_index();
    void read_config();
    

    std::string user;
    std::string pass;
    std::string db_name;
    std::string host;
    std::string port;
    pqxx::connection *C;

};
#endif
