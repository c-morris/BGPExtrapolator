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

#include "SQLQueriers/ROVppSQLQuerier.h"
#include "TableNames.h"

/** Constructor
 */
ROVppSQLQuerier::ROVppSQLQuerier(std::vector<std::string> policy_tables /* = std::vector<std::string>() */,
                                    std::string announcement_table /* = ROVPP_ANNOUNCEMENTS_TABLE */,
                                    std::string results_table /* = ROVPP_RESULTS_TABLE */,
                                    std::string inverse_results_table /* = INVERSE_RESULTS_TABLE */, 
                                    std::string depref_results_table /* = DEPREF_RESULTS_TABLE */,
                                    std::string full_path_results_table /* = FULL_PATH_RESULTS_TABLE */,
                                    std::string tracked_ases_table /* = TRACKED_ASES_TABLE */,
                                    std::string simulation_table /* = ROVPP_SIMULATION_TABLE */,
                                    int exclude_as_number /* = -1 */,
                                    std::string config_section /* = "bgp" */)
    : SQLQuerier<>(announcement_table, results_table, inverse_results_table, depref_results_table, full_path_results_table, exclude_as_number, config_section) {
    
    this->policy_tables = policy_tables;
    this->results_table = results_table;
    this->tracked_ases_table = tracked_ases_table;
    this->simulation_table = simulation_table;
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
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin FROM " + cur_table;
    sql += " WHERE prefix = \'" + cidr + "\';";
    return execute(sql);
}

/** Pulls all victim or attacker pairs for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining a whole subnet for which we select
 */
pqxx::result ROVppSQLQuerier::select_subnet_pairs(Prefix<>* p, std::string const& cur_table){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin FROM " + cur_table;
    sql += " WHERE prefix <<= \'" + cidr + "\';";
    return execute(sql);
}

/** Pulls in victim or attacker pairs.
 * 
 * @param cur_table This is a constant value you can find in TableNames.h (either ATTACKER_TABLE or VICTIM_TABLE) 
 * @return         Database results as [prefix, netmask, as_path, origin, policy_index]
 */
pqxx::result ROVppSQLQuerier::select_all_pairs_from(std::string const& cur_table){
    std::string sql = "SELECT host(prefix) AS prefix_host, netmask(prefix) AS prefix_netmask, as_path, origin, prefix_id, block_prefix_id FROM " + cur_table;
    return execute(sql);
}

/** Pulls list of attackers and victims from tracked_ases table.
 *
 * @param cur_table Tracked ASes table name
 */
pqxx::result ROVppSQLQuerier::select_tracked_ases(std::string const& cur_table){
    std::string sql = "SELECT asn, attacker FROM " + cur_table;
    return execute(sql);
}



/** Takes a .csv filename and bulk copies all elements to the results table.
 */
void ROVppSQLQuerier::copy_results_to_db(std::string file_name){
    std::string sql = std::string("COPY " + results_table + "(asn, prefix, origin, received_from_asn, time, alternate_as)") +
                                  "FROM '" + file_name + "' WITH (FORMAT csv)";
    execute(sql);
}

/** Instantiates a new, empty results table in the database, dropping the old table.
 */
void ROVppSQLQuerier::create_results_tbl(){
    std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + results_table + " (\
    asn bigint,prefix cidr, origin bigint, received_from_asn \
    bigint, time bigint, alternate_as bigint, opt_flag int); GRANT ALL ON TABLE " + results_table + " TO bgp_user;");
    BOOST_LOG_TRIVIAL(info) << "Creating " << results_table << " table...";
    execute(sql, false);
}

/**
 * Copies the blackholes from each AS into DB table
 * @param file_name the name of the file csv that contains the blackholes information
 */
void ROVppSQLQuerier::copy_blackhole_list_to_db(std::string file_name) {
  // std::string sql = std::string("COPY " ROVPP_BLACKHOLES_TABLE "(asn, prefix, origin, received_from_asn)") +
  std::string sql = std::string("COPY " ROVPP_BLACKHOLES_TABLE "(asn, prefix, origin, received_from_asn, tstamp)") +
                      "FROM '" + file_name + "' WITH (FORMAT csv)";
  execute(sql);
}


/**
 * Creates an empty ROVPP_BLACKHOLES_TABLE.
 */
void ROVppSQLQuerier::create_rovpp_blacklist_tbl() {
  // Drop the results table
  std::string sql = std::string("DROP TABLE IF EXISTS " ROVPP_BLACKHOLES_TABLE " ;");
  BOOST_LOG_TRIVIAL(info) << "Dropping " ROVPP_BLACKHOLES_TABLE " table...";
  execute(sql, false);
  // Create it again
  std::string sql2 = std::string("CREATE TABLE IF NOT EXISTS " ROVPP_BLACKHOLES_TABLE "(asn BIGINT, prefix CIDR, origin BIGINT, received_from_asn BIGINT, tstamp BIGINT)");
  BOOST_LOG_TRIVIAL(info) << "Creating " ROVPP_BLACKHOLES_TABLE " table...";
  execute(sql2, false);
}
