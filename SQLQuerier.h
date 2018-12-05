#ifndef SQLQUERIER_H
#define SQLQUERIER_H

#include <pqxx/pqxx>
#include <iostream>

struct SQLQuerier {
    SQLQuerier();
    void test_connection();
    void read_config();
};
#endif
