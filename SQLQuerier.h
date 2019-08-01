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

struct SQLQuerier {
    SQLQuerier(std::string a=ANNOUNCEMENTS_TABLE, 
               std::string r=RESULTS_TABLE,
               std::string i=INVERSE_RESULTS_TABLE,
               std::string d=DEPREF_RESULTS_TABLE,
               std::string v=VERIFICATION_TABLE);
    ~SQLQuerier();
    
    // Setup
    void read_config();
    void open_connection();
    void close_connection();
    pqxx::result execute(std::string sql, bool insert = false);
    
    // Select from DB
    pqxx::result select_from_table(std::string table_name, int limit = 0);
    pqxx::result select_prefix_count(Prefix<>*);
    pqxx::result select_prefix_ann(Prefix<>*);
    pqxx::result select_subnet_count(Prefix<>*);
    pqxx::result select_subnet_ann(Prefix<>*);
    
    // Preprocessing Tables
    void clear_stubs_from_db();
    void clear_non_stubs_from_db();
    void clear_supernodes_from_db();
    void clear_vf_from_db();
    
    void create_stubs_tbl();
    void create_non_stubs_tbl();
    void create_supernodes_tbl();
    void create_vf_tbl();
    
    void copy_stubs_to_db(std::string file_name);
    void copy_non_stubs_to_db(std::string file_name);
    void copy_supernodes_to_db(std::string file_name);
    void insert_vf_ann_to_db(uint32_t, std::string, uint32_t, std::string);
    
    // Propagation Tables
    void clear_results_from_db();
    void clear_depref_from_db();
    void clear_inverse_from_db();

    void create_results_tbl();
    void create_depref_tbl();
    void create_inverse_results_tbl();
 
    void copy_results_to_db(std::string file_name);
    void copy_depref_to_db(std::string file_name);
    void copy_inverse_results_to_db(std::string file_name);
    
    void create_results_index();
    
private:
    std::string results_table;
    std::string depref_table;
    std::string inverse_results_table;
    std::string verification_table;
    std::string announcements_table;
    std::string user;
    std::string pass;
    std::string db_name;
    std::string host;
    std::string port;
    pqxx::connection *C;
};
#endif
