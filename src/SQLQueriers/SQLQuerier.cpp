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

#include "SQLQueriers/SQLQuerier.h"

SQLQuerier::SQLQuerier(std::string announcements_table /* = ANNOUNCEMENTS_TABLE */,
                        std::string results_table /* = RESULTS_TABLE */, 
                        std::string inverse_results_table /* = INVERSE_RESULTS_TABLE */, 
                        std::string depref_results_table /* = DEPREF_RESULTS_TABLE */,
                        std::string config_section,
                        std::string config_path,
                        bool create_connection) {
    this->announcements_table = announcements_table;
    this->results_table = results_table;
    this->depref_table = depref_results_table;
    this->inverse_results_table = inverse_results_table;
    this->config_section = config_section;
    this->config_path = config_path;
    
    // Default host and port numbers
    // Strings for connection arg
    host = "127.0.0.1";
    port = "5432";

    read_config();

    if (create_connection){
        open_connection();
    }
}

SQLQuerier::~SQLQuerier() {
    C->disconnect();
    delete C;
}


/** Reads credentials/connection info from .conf file
 */
void SQLQuerier::read_config() {
    using namespace std;

    cout << "Config section: " << config_section << std::endl;
    
    program_options::variables_map var_map;

    // Specify config parameters to parse
    program_options::options_description file_options("File options");
    file_options.add_options()
        ((config_section + ".user").c_str(), program_options::value<string>(), "username")
        ((config_section + ".password").c_str(), program_options::value<string>(), "password")
        ((config_section + ".database").c_str(), program_options::value<string>(), "db")
        ((config_section + ".host").c_str(), program_options::value<string>(), "host")
        ((config_section + ".port").c_str(), program_options::value<string>(), "port")
        ;

    program_options::notify(var_map);

    ifstream cFile(config_path);
    if (cFile.is_open()) {
        // Map config variables to settings in file
        program_options::store(program_options::parse_config_file(cFile, file_options, true), var_map);

        if (var_map.count(config_section + ".user")){
            user = var_map[config_section + ".user"].as<string>();
        }

        if (var_map.count(config_section + ".password")){
            pass = var_map[config_section + ".password"].as<string>();
        }

        if (var_map.count(config_section + ".database")){
            db_name = var_map[config_section + ".database"].as<string>();
        }

        if (var_map.count(config_section + ".host")){
            if (var_map[config_section + ".host"].as<string>() == "localhost") {
                host = "127.0.0.1";
            } else {
                host = var_map[config_section + ".host"].as<string>();
            }
        }

        if (var_map.count(config_section + ".port")){
            port = var_map[config_section + ".port"].as<string>();
        }
    } else {
        std::cerr << "Error loading config file \"" << config_path << "\"" << std::endl;
    }
}


/** Opens a connection to the SQL database.
 */
void SQLQuerier::open_connection() {
    std::ostringstream stream;
    stream << "dbname = " << db_name;
    stream << " user = " << user;
    stream << " password = " << pass;
    stream << " hostaddr = " << host;
    stream << " port = " << port;
    // Try connecting with Querier object settings
    try {
        pqxx::connection *conn = new pqxx::connection(stream.str());
        if (conn->is_open()) {
            C = conn;
        } else {
            std::cerr << "Failed to connect to database : " << db_name <<std::endl;
            return;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}


/** Closes the connection to the SQL database.
 */
void SQLQuerier::close_connection() {
    C->disconnect();
}


/** Executes a give SQL statement
 *
 *  @param sql
 *  @param insert
 */
pqxx::result SQLQuerier::execute(std::string sql, bool insert) {
    pqxx::result R;
    if(insert){
        //TODO maybe make one work object once on construction
        //work object may be same as nontransaction with more functionality
        try {
            pqxx::work txn(*C);
            R = txn.exec(sql);
            txn.commit();
            return R;
        } catch(const std::exception &e) {
            std::cerr << e.what() <<std::endl;
        }
    } else {
        try {
            pqxx::nontransaction N(*C);
            pqxx::result R( N.exec(sql));
            return R;
        } catch(const std::exception &e) {
            std::cerr << e.what() <<std::endl;
        }
    }
    return R;
}


/** Generic SELECT query for returning the entire relationship tables.
 *
 *  @param table_name The name of the table to SELECT from
 *  @param limit The limit of the number of values to SELECT
 */
pqxx::result SQLQuerier::select_from_table(std::string table_name, int limit) {
    std::string sql = "SELECT * FROM " + table_name;
    if (limit) {
        sql += " LIMIT " + std::to_string(limit);
    }
    return execute(sql);
}


/** Pulls the count for all announcements for the prefix.
 *
 * @param p The prefix for which we SELECT
 */
pqxx::result SQLQuerier::select_prefix_count(Prefix<>* p) {
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT COUNT(*) FROM " + announcements_table;
    sql += " WHERE prefix = \'" + cidr + "\';";
    return execute(sql);
}


/** Pulls all announcements for the announcements for the given prefix.
 *
 * @param p The prefix for which we SELECT
 */
pqxx::result SQLQuerier::select_prefix_ann(Prefix<>* p) {
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin, time, prefix_id, block_prefix_id FROM " + announcements_table;
    sql += " WHERE prefix = \'" + cidr + "\';";
    return execute(sql);
}


/** Pulls the count for all announcements for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
pqxx::result SQLQuerier::select_subnet_count(Prefix<>* p) {
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT COUNT(*) FROM " + announcements_table;
    sql += " WHERE prefix <<= \'" + cidr + "\';";
    return execute(sql);
}


/** Pulls all announcements for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
pqxx::result SQLQuerier::select_subnet_ann(Prefix<>* p) {
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin, time, prefix_id, block_prefix_id FROM " + announcements_table;
    sql += " WHERE prefix <<= \'" + cidr + "\';";
    return execute(sql);
}


/** this should use the STUBS_TABLE macro
 */
void SQLQuerier::clear_stubs_from_db() {
    std::string sql = std::string("DROP TABLE IF EXISTS " STUBS_TABLE ";");
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the stubs table.
 */
void SQLQuerier::clear_non_stubs_from_db() {
    std::string sql = std::string("DROP TABLE IF EXISTS " NON_STUBS_TABLE ";");
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the stubs table.
 */
void SQLQuerier::clear_supernodes_from_db() {
    std::string sql = std::string("DROP TABLE IF EXISTS " SUPERNODES_TABLE ";");
    execute(sql);
}


/** Instantiates a new, empty stubs table in the database, if it doesn't exist.
 */
void SQLQuerier::create_stubs_tbl() {
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS " STUBS_TABLE " (stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint);");
    std::cout << "Creating stubs table..." << std::endl;
    execute(sql, false);
}


/** Instantiates a new, empty non_stubs table in the database, if it doesn't exist.
 */
void SQLQuerier::create_non_stubs_tbl() {
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS " NON_STUBS_TABLE " (non_stub_asn BIGSERIAL PRIMARY KEY);");
    std::cout << "Creating non_stubs table..." << std::endl;
    execute(sql, false);
}


/**  Instantiates a new, empty supernodes table in the database, if it doesn't exist.
 */
void SQLQuerier::create_supernodes_tbl() {
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS " SUPERNODES_TABLE "(supernode_asn BIGSERIAL PRIMARY KEY, supernode_lowest_asn bigint)");
    std::cout << "Creating supernodes table..." << std::endl;
    execute(sql, false);
}


/** Takes a .csv filename and bulk copies all elements to the stubs table.
 */
void SQLQuerier::copy_stubs_to_db(std::string file_name) {
    std::string sql = "COPY " STUBS_TABLE "(stub_asn,parent_asn) FROM '" +
                      file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the non-stubs table.
 */
void SQLQuerier::copy_non_stubs_to_db(std::string file_name) {
    std::string sql = "COPY " NON_STUBS_TABLE "(non_stub_asn) FROM '" +
                      file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the supernodes table.
 */
void SQLQuerier::copy_supernodes_to_db(std::string file_name) {
    std::string sql = "COPY " SUPERNODES_TABLE "(supernode_asn,supernode_lowest_asn) FROM '" +
                      file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Drop the Querier's results table.
 */
void SQLQuerier::clear_results_from_db() {
    std::string sql = std::string("DROP TABLE IF EXISTS " + results_table + ";");
    execute(sql);
}


/** Drop the Querier's depref table.
 */
void SQLQuerier::clear_depref_from_db() {
    std::string sql = std::string("DROP TABLE IF EXISTS " + depref_table + ";");
    execute(sql);
}


/** Drop the Querier's inverse results table.
 */
void SQLQuerier::clear_inverse_from_db() {
    std::string sql = std::string("DROP TABLE IF EXISTS " + inverse_results_table + ";");
    execute(sql);
}


/** Instantiates a new, empty results table in the database, dropping the old table.
 */
void SQLQuerier::create_results_tbl() {
    /**
    std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + results_table + " (ann_id serial PRIMARY KEY,\
    */
    std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + results_table + " (\
    asn bigint,prefix cidr,prefix_id bigint, origin bigint, received_from_asn \
    bigint, time bigint); GRANT ALL ON TABLE " + results_table + " TO bgp_user;");
    std::cout << "Creating results table..." << std::endl;
    execute(sql, false);
}


/** Instantiates a new, empty depref table in the database, dropping the old table.
 */
void SQLQuerier::create_depref_tbl() {
    std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + depref_table + " (\
    asn bigint,prefix cidr,prefix_id bigint, origin bigint, received_from_asn \
    bigint, time bigint); GRANT ALL ON TABLE " + depref_table + " TO bgp_user;");
    std::cout << "Creating depref table..." << std::endl;
    execute(sql, false);
}


/** Instantiates a new, empty inverse results table in the database, dropping the old table.
 */
void SQLQuerier::create_inverse_results_tbl() {
    std::string sql;
    sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS ") + inverse_results_table + 
    "(asn bigint,prefix cidr,prefix_id bigint, origin bigint) ";
    sql += ";";
    sql += "GRANT ALL ON TABLE " + inverse_results_table + " TO bgp_user;";
    std::cout << "Creating inverse results table..." << std::endl;
    execute(sql, false);
}


/** Takes a .csv filename and bulk copies all elements to the results table.
 */
void SQLQuerier::copy_results_to_db(std::string file_name) {
    std::string sql = std::string("COPY " + results_table + "(asn, prefix, prefix_id, origin, received_from_asn, time)") +
                                  "FROM '" + file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the depref table.
 */
void SQLQuerier::copy_depref_to_db(std::string file_name) {
    std::string sql = std::string("COPY " + depref_table + "(asn, prefix,prefix_id, origin, received_from_asn, time)") +
                                  "FROM '" + file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the inverse results table.
 */
void SQLQuerier::copy_inverse_results_to_db(std::string file_name) {
    std::string sql = std::string("COPY " + inverse_results_table + "(asn, prefix,prefix_id, origin)") +
                                  "FROM '" + file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Generate an index on the results table.
 */
void SQLQuerier::create_results_index() {
    // Version of postgres must support this
    std::string sql = std::string("CREATE INDEX ON " + results_table + " USING GIST(prefix inet_ops, origin)");
    std::cout << "Generating index on results..." << std::endl;
    execute(sql, false);
}
