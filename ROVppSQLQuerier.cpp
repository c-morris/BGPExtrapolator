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


#include "ROVppSQLQuerier.h"

// need function to read in sql table(s) of attacker-victim pairs
// should get justin's input on how to do the tables

// also need to read in rov validity (or a list/set of attackers)

/** Constructor
 */
ROVppSQLQuerier::ROVppSQLQuerier(std::string a, std::string r, std::string i, std::string d)
    : SQLQuerier(a, r, i, d) {
}

ROVppSQLQuerier::~ROVppSQLQuerier() {
}


/** Pulls all announcements for the prefixes contained within the passed subnet.
 *
 * @param flag_table The table name holding the list of ASN to flag
 */
pqxx::result ROVppSQLQuerier::select_AS_flag(std::string const& flag_table){
    std::string sql = "SELECT asn FROM " + flag_table + " WHERE impliments = t;";
    return execute(sql);
}

/** Pulls all victim or attacker pairs for the announcements for the given prefix.
 *
 * @param p The prefix for which we SELECT
 */
pqxx::result ROVppSQLQuerier::select_prefix_pairs(Prefix<>* p, std::string const& cur_table){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin FROM " + cur_table;
    sql += " WHERE prefix = \'" + cidr + "\';";
    return execute(sql);
}

/** Pulls all victim or attacker pairs for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
pqxx::result ROVppSQLQuerier::select_subnet_pairs(Prefix<>* p, std::string const& cur_table){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin FROM " + cur_table;
    sql += " WHERE prefix <<= \'" + cidr + "\';";
    return execute(sql);
}
