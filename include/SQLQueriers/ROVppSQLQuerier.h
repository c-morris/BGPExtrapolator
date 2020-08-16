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

#ifndef ROVPP_SQLQUERIER_H
#define ROVPP_SQLQUERIER_H

#include "SQLQueriers/SQLQuerier.h"

class ROVppSQLQuerier: public SQLQuerier {
public:
    std::string simulation_table;
    std::string tracked_ases_table;
    std::vector<std::string> policy_tables;
    
    ROVppSQLQuerier(std::vector<std::string> policy_tables,
                    std::string announcement_table = ROVPP_ANNOUNCEMENTS_TABLE,
                    std::string results_table = ROVPP_RESULTS_TABLE,
                    std::string inverse_results_table = INVERSE_RESULTS_TABLE, 
                    std::string depref_results_table = DEPREF_RESULTS_TABLE,
                    std::string tracked_ases_table = TRACKED_ASES_TABLE,
                    std::string simulation_table = ROVPP_SIMULATION_TABLE);
    ~ROVppSQLQuerier();

    pqxx::result select_AS_flags(std::string const& flag_table = std::string("rovpp_ases"));
    pqxx::result select_prefix_pairs(Prefix<>* p, std::string const& cur_table);
    pqxx::result select_subnet_pairs(Prefix<>* p, std::string const& cur_table);
    pqxx::result select_all_pairs_from(std::string const& cur_table);
    pqxx::result select_tracked_ases(std::string const& cur_table);
    
    void copy_results_to_db(std::string);
    void create_results_tbl();
    void copy_blackhole_list_to_db(std::string file_name);
    void create_rovpp_blacklist_tbl();
};
#endif
