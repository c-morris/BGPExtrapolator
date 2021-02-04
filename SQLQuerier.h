/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#ifndef SQLQUERIER_H
#define SQLQUERIER_H

#define IPV4 4
#define IPV6 6

#define DEFAULT_QUERIER_CONFIG_SECTION "bgp"

class ASGraph;

#include <pqxx/pqxx>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "ASGraph.h"
#include "TableNames.h"

#include <boost/program_options.hpp>
namespace program_options = boost::program_options;

class SQLQuerier {
public:
    std::string results_table;
    std::string depref_table;
    std::string inverse_results_table;
    std::string announcements_table;
    std::string user;
    std::string pass;
    std::string db_name;
    std::string host;
    std::string port;
    std::string config_section;
    std::string config_path;
    pqxx::connection *C;

    SQLQuerier(std::string a=ANNOUNCEMENTS_TABLE, 
               std::string r=RESULTS_TABLE,
               std::string i=INVERSE_RESULTS_TABLE,
               std::string d=DEPREF_RESULTS_TABLE,
               std::string cs = DEFAULT_QUERIER_CONFIG_SECTION,
               std::string cp = "/etc/bgp/bgp.conf",
               bool create_connection = true);
    virtual ~SQLQuerier();
    
    // Setup
    void read_config();
    void open_connection();
    void close_connection();
    pqxx::result execute(std::string sql, bool insert = false);
    
    // Select from DB
    pqxx::result select_from_table(std::string table_name, int limit = 0);
    pqxx::result select_prefix_count(Prefix<>*);
    virtual pqxx::result select_prefix_ann(Prefix<>*);
    pqxx::result select_subnet_count(Prefix<>*);
    virtual pqxx::result select_subnet_ann(Prefix<>*);
    
    // Preprocessing Tables
    void clear_stubs_from_db();
    void clear_non_stubs_from_db();
    void clear_supernodes_from_db();
    
    void create_stubs_tbl();
    void create_non_stubs_tbl();
    void create_supernodes_tbl();
    
    void copy_stubs_to_db(std::string file_name);
    void copy_non_stubs_to_db(std::string file_name);
    void copy_supernodes_to_db(std::string file_name);
    
    // Propagation Tables
    void clear_results_from_db();
    void clear_depref_from_db();
    void clear_inverse_from_db();

    virtual void create_results_tbl();
    void create_depref_tbl();
    void create_inverse_results_tbl();
 
    virtual void copy_results_to_db(std::string file_name);
    void copy_depref_to_db(std::string file_name);
    void copy_inverse_results_to_db(std::string file_name);
    
    void create_results_index();
};
#endif
