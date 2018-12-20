#ifndef SQLQUERIER_H
#define SQLQUERIER_H

#include <pqxx/pqxx>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "ASGraph.h"

struct SQLQuerier {
    SQLQuerier();
    ~SQLQuerier();

    void open_connection();
    void close_connection();
    pqxx::result execute(std::string sql);
    pqxx::result select_from_table(std::string table_name, int limit = 0);
    pqxx::result select_ann_records(std::string table_name, std::string prefix = "", int limit = 0);
    pqxx::result select_ann_records(std::string table_name, std::vector<std::string> prefixes, int limit = 0);
    pqxx::result select_distinct_prefixes_from_table(std::string table_name);
    void insert_results(ASGraph* graph);
    void read_config();
    

    std::string user;
    std::string pass;
    std::string db_name;
    std::string host;
    std::string port;
    pqxx::connection *C;

};
#endif
