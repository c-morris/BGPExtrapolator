#ifndef SQLQUERIER_H
#define SQLQUERIER_H

#include <pqxx/pqxx>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

struct SQLQuerier {
    SQLQuerier();
    void test_connection();
    void read_config();

    std::string user;
    std::string pass;
    std::string db_name;
    std::string host;
    std::string port;
    
};
#endif
