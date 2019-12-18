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
#include "TableNames.h"

/** Constructor
 */
ROVppSQLQuerier::ROVppSQLQuerier(std::string e, 
                                 std::string f, 
                                 std::string g, 
                                 std::string h, 
                                 std::string j) 
                                 : SQLQuerier() {
    // TODO allow output table name specification
    victim_table = e;
    attack_table = f;
    top_table = g;
    etc_table = h;
    edge_table = j;
}

ROVppSQLQuerier::~ROVppSQLQuerier() {
}


/** Pulls all announcements for the prefixes contained within the passed subnet.
 *
 * Default flag table name is 'rovpp_ases'.
 *
 * @param flag_table The table name holding the list of ASN to flag
 */
pqxx::result ROVppSQLQuerier::select_AS_flags(std::string const& flag_table){
    std::string sql = "SELECT asn, as_type, impliment FROM " + flag_table + ";";
    return execute(sql);
}

/** Pulls all victim or attacker pairs for the announcements for the given prefix.
 *
 * @param p The prefix for which we specifically select.
 */
pqxx::result ROVppSQLQuerier::select_prefix_pairs(Prefix<>* p, std::string const& cur_table){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin, policy_index FROM " + cur_table;
    sql += " WHERE prefix = \'" + cidr + "\';";
    return execute(sql);
}

/** Pulls all victim or attacker pairs for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining a whole subnet for which we select
 */
pqxx::result ROVppSQLQuerier::select_subnet_pairs(Prefix<>* p, std::string const& cur_table){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin, policy_index FROM " + cur_table;
    sql += " WHERE prefix <<= \'" + cidr + "\';";
    return execute(sql);
}


/** Pulls in victim or attacker pairs.
 * 
 * @param cur_table This is a constant value you can find in TableNames.h (either ATTACKER_TABLE or VICTIM_TABLE) 
 * @return         Database results as [prefix, netmask, as_path, origin, policy_index]
 */
pqxx::result ROVppSQLQuerier::select_all_pairs_from(std::string const& cur_table){
    std::string sql = "SELECT host(prefix) AS prefix_host, netmask(prefix) AS prefix_netmask, as_path, origin, policy_index FROM " + cur_table;
    return execute(sql);
}

/** Takes a .csv filename and bulk copies all elements to the results table.
 */
void ROVppSQLQuerier::copy_results_to_db(std::string file_name){
    std::string sql = std::string("COPY " + results_table + "(asn, prefix, origin, received_from_asn, opt_flag)") +
                                  "FROM '" + file_name + "' WITH (FORMAT csv)";
    execute(sql);
}

