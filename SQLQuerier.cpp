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

#include "SQLQuerier.h"

SQLQuerier::SQLQuerier(std::string a, 
                       std::string r, 
                       std::string i, 
                       std::string d,
                       std::string v) {
    announcements_table = a;
    results_table = r;
    depref_table = d;
    inverse_results_table = i;
    verification_table = v;
    
    // Default host and port numbers
    // Strings for connection arg
    host = "127.0.0.1";
    port = "5432";

    read_config();
    open_connection();
}

SQLQuerier::~SQLQuerier(){
    C->disconnect();
    delete C;
}


/** Reads credentials/connection info from .conf file
 */
void SQLQuerier::read_config(){
    using namespace std;
    string file_location = "/etc/bgp/bgp.conf";
    ifstream cFile(file_location);
    if (cFile.is_open()) {
        // Map config variables to settings in file
        map<string,string> config;
        string line;
        
        while(getline(cFile, line)) {
            // Remove whitespace and check to ignore line
            line.erase(remove_if(line.begin(),line.end(),::isspace), line.end());
            if (line.empty() || line[0] == '#' || line[0] == '[') {
                continue;
            }
            auto delim_index = line.find("=");
            std::string var_name = line.substr(0,delim_index);
            std::string value = line.substr(delim_index+1);
            config.insert(std::pair<std::string,std::string>(var_name, value));
        }

        for (auto const& setting : config){
            if(setting.first == "user") {
                user = setting.second;
            } else if(setting.first == "password") {
                pass = setting.second;
            } else if(setting.first == "database") {
                db_name = setting.second;
            } else if(setting.first == "host") {
                if (setting.second == "localhost") {
                    host = "127.0.0.1";
                } else {
                    host = setting.second;
                }
            } else if(setting.first == "port") {
                port = setting.second;
            } else {
                // This outputs extraneous setting found in the config file
                // std::cerr << "Setting \"" << setting.first << "\" undefined." << std::endl;
            }
        }
    } else {
        std::cerr << "Error loading config file \"" << file_location << "\"" << std::endl;
    }
}


/** Opens a connection to the SQL database.
 */
void SQLQuerier::open_connection(){
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
            std::cout << "Connected to database: " << db_name <<std::endl;
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
void SQLQuerier::close_connection(){
    C->disconnect();
}


/** Executes a give SQL statement
 *
 *  @param sql
 *  @param insert
 */
pqxx::result SQLQuerier::execute(std::string sql, bool insert){
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
pqxx::result SQLQuerier::select_from_table(std::string table_name, int limit){
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
pqxx::result SQLQuerier::select_prefix_count(Prefix<>* p){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT COUNT(*) FROM " + announcements_table;
    sql += " WHERE prefix = \'" + cidr + "\';";
    return execute(sql);
}


/** Pulls all announcements for the announcements for the given prefix.
 *
 * @param p The prefix for which we SELECT
 */
pqxx::result SQLQuerier::select_prefix_ann(Prefix<>* p){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin, time FROM " + announcements_table;
    sql += " WHERE prefix = \'" + cidr + "\' ORDER BY time ASC;";
    return execute(sql);
}


/** Pulls the count for all announcements for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
pqxx::result SQLQuerier::select_subnet_count(Prefix<>* p){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT COUNT(*) FROM " + announcements_table;
    sql += " WHERE prefix <<= \'" + cidr + "\';";
    return execute(sql);
}


/** Pulls all announcements for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
pqxx::result SQLQuerier::select_subnet_ann(Prefix<>* p){
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin, time FROM " + announcements_table;
    sql += " WHERE prefix <<= \'" + cidr + "\' ORDER BY time ASC;";
    return execute(sql);
}


/** Clears all rows from the stubs table.
 */
void SQLQuerier::clear_stubs_from_db(){
    std::string sql = std::string("DROP TABLE IF EXISTS " STUBS_TABLE ";");
    execute(sql);
}


/** Clears all rows from the non-stubs table.
 */
void SQLQuerier::clear_non_stubs_from_db(){
    std::string sql = std::string("DROP TABLE IF EXISTS " NON_STUBS_TABLE ";");
    execute(sql);
}


/** Clears all rows from the supernodes table.
 */
void SQLQuerier::clear_supernodes_from_db(){
    std::string sql = std::string("DROP TABLE IF EXISTS " SUPERNODES_TABLE ";");
    execute(sql);
}


/** Clears all rows from the verification control table. 
 */
void SQLQuerier::clear_vf_from_db(){
    std::string sql = std::string("DELETE FROM " + verification_table + ";");
    execute(sql);
}


/** Instantiates a new, empty stubs table in the database, if it doesn't exist.
 */
void SQLQuerier::create_stubs_tbl(){
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS " STUBS_TABLE " (stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint);");
    std::cout << "Creating stubs table..." << std::endl;
    execute(sql, false);
}


/** Instantiates a new, empty non_stubs table in the database, if it doesn't exist.
 */
void SQLQuerier::create_non_stubs_tbl(){
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS " NON_STUBS_TABLE " (non_stub_asn BIGSERIAL PRIMARY KEY);");
    std::cout << "Creating non_stubs table..." << std::endl;
    execute(sql, false);
}


/**  Instantiates a new, empty supernodes table in the database, if it doesn't exist.
 */
void SQLQuerier::create_supernodes_tbl(){
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS " SUPERNODES_TABLE "(supernode_asn BIGSERIAL PRIMARY KEY, supernode_lowest_asn bigint)");
    std::cout << "Creating supernodes table..." << std::endl;
    execute(sql, false);
}


/**  Instantiates a new, empty verification control table in the database, if it doesn't exist.
 */
void SQLQuerier::create_vf_tbl(){
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS " + verification_table + "(monitor_as bigint, prefix cidr, origin bigint, as_path bigint[])");
    std::cout << "Creating verification table..." << std::endl;
    execute(sql, false);
}


/** Takes a .csv filename and bulk copies all elements to the stubs table.
 */
void SQLQuerier::copy_stubs_to_db(std::string file_name){
    std::string sql = "COPY " STUBS_TABLE "(stub_asn,parent_asn) FROM '" 
                      + file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the non-stubs table.
 */
void SQLQuerier::copy_non_stubs_to_db(std::string file_name){
    std::string sql = "COPY " NON_STUBS_TABLE "(non_stub_asn) FROM '" 
                      + file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the supernodes table.
 */
void SQLQuerier::copy_supernodes_to_db(std::string file_name){
    std::string sql = "COPY " SUPERNODES_TABLE "(supernode_asn,supernode_lowest_asn) FROM '" 
                      + file_name + "' WITH (FORMAT csv)";
    execute(sql);
}


/** Inserts rows into the verification table.
 */
void SQLQuerier::insert_vf_ann_to_db(uint32_t monitor, std::string prefix, uint32_t origin, std::string path){
    std::string sql = "INSERT INTO " + verification_table + " (monitor_as, prefix, origin, as_path) VALUES (" 
                      + std::to_string(monitor) + ", "
                      + "\'" + prefix + "\', "
                      + std::to_string(origin) + ", "
                      + "\'" + path + "\')";
    execute(sql, true);
}



/** Drop the Querier's results table.
 */
void SQLQuerier::clear_results_from_db(){
    std::string sql = std::string("DROP TABLE IF EXISTS " + results_table + ";");
    execute(sql);
}


/** Drop the Querier's depref table.
 */
void SQLQuerier::clear_depref_from_db(){
    std::string sql = std::string("DROP TABLE IF EXISTS " + depref_table + ";");
    execute(sql);
}


/** Drop the Querier's inverse results table.
 */
void SQLQuerier::clear_inverse_from_db(){
    std::string sql = std::string("DROP TABLE IF EXISTS " + inverse_results_table + ";");
    execute(sql);
}


/** Instantiates a new, empty results table in the database, dropping the old table.
 */
void SQLQuerier::create_results_tbl(){
    std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + results_table 
                      + " (asn bigint, prefix cidr, origin bigint, as_path bigint[], time bigint,\
                      inference_l smallint); GRANT ALL ON TABLE " + results_table + " TO bgp_user;");
    std::cout << "Creating results table..." << std::endl;
    execute(sql, false);
}


/** Instantiates a new, empty depref table in the database, dropping the old table.
 */
void SQLQuerier::create_depref_tbl(){
    std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + depref_table 
                      + " (asn bigint,prefix cidr, origin bigint, as_path bigint[], time bigint,\
                      inference_l smallint); GRANT ALL ON TABLE " + depref_table + " TO bgp_user;");
    std::cout << "Creating depref table..." << std::endl;
    execute(sql, false);
}


/** Instantiates a new, empty inverse results table in the database, dropping the old table.
 */
void SQLQuerier::create_inverse_results_tbl(){
    std::string sql;
    sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS ") 
          + inverse_results_table 
          + "(asn bigint,prefix cidr, origin bigint);";
          + "GRANT ALL ON TABLE " + inverse_results_table + " TO bgp_user;";
    std::cout << "Creating inverse results table..." << std::endl;
    execute(sql, false);
}


/** Takes a .csv filename and bulk copies all elements to the results table.
 */
void SQLQuerier::copy_results_to_db(std::string file_name){
    std::string sql = std::string("COPY " + results_table 
                      + "(asn, prefix, origin, as_path, time, inference_l)"
                      + "FROM '" + file_name + "' WITH (FORMAT csv)");
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the depref table.
 */
void SQLQuerier::copy_depref_to_db(std::string file_name){
    std::string sql = std::string("COPY " + depref_table 
                      + "(asn, prefix, origin, as_path, time, inference_l)"
                      + "FROM '" + file_name + "' WITH (FORMAT csv)");
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the inverse results table.
 */
void SQLQuerier::copy_inverse_results_to_db(std::string file_name){
    std::string sql = std::string("COPY " + inverse_results_table 
                      + "(asn, prefix, origin)"
                      + "FROM '" + file_name + "' WITH (FORMAT csv)");
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
