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
                        int exclude_as_number, /* = -1 */
                        std::string config_section /* = "bgp" */,
                        std::string config_path /* = "/etc/bgp/bgp.conf" */,
                        bool create_connection /* = true */) {
    this->announcements_table = announcements_table;
    this->results_table = results_table;
    this->depref_table = depref_results_table;
    this->inverse_results_table = inverse_results_table;
    this->config_section = config_section;
    this->config_path = config_path;
    this->exclude_as_number = exclude_as_number;
    
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

    BOOST_LOG_TRIVIAL(info) << "Config section: " << config_section;
    
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
        BOOST_LOG_TRIVIAL(error) << "Error loading config file \"" << config_path << "\"";
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
            BOOST_LOG_TRIVIAL(error) << "Failed to connect to database : " << db_name;
            return;
        }
    } catch (const std::exception &e) {
        BOOST_LOG_TRIVIAL(error) << e.what();
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
            BOOST_LOG_TRIVIAL(error) << e.what();
        }
    } else {
        try {
            pqxx::nontransaction N(*C);
            pqxx::result R( N.exec(sql));
            return R;
        } catch(const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << e.what();
        }
    }
    return R;
}

/** Returns a string with SQL COPY query
 *
 *  @param file_name The name of the file to COPY from
 *  @param table_name The name of the table to COPY to
 *  @param column_names Comma separated list of column names to be copied,
 *                      surrounded by parentheses, Ex: (stub_asn,parent_asn)
 */
std::string SQLQuerier::copy_to_db_string(std::string file_name, std::string table_name, std::string column_names) {
    std::string sql = "COPY " + table_name + column_names + " FROM '" +
                      file_name + "' WITH (FORMAT csv)";

    return sql;
}

/** Returns a string with SQL SELECT query for announcements within prefix / subnet
 *
 *  @param p The prefix for which we SELECT
 *  @param subnet True - SELECT the announcements the prefixes contained within the passed subnet
 *                False - SELECT announcements for the prefix
 *  @param selection Comma separated list of column names to be selected,
 *                      Ex: host(prefix), netmask(prefix), as_path, origin, time
 */
std::string SQLQuerier::select_prefix_string(Prefix<>* p, bool subnet, std::string selection) {
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT " + selection + " FROM " + announcements_table;
    if (subnet) {
        sql += " WHERE prefix <<= \'" + cidr + "\'" + exclude_asn_string();
    } else {
        sql += " WHERE prefix = \'" + cidr + "\'" + exclude_asn_string();
    }

    return sql;
}

/** Returns a string with ASN that needs to be excluded
 *  If there isn't one, returns a semicolon
 *  This helps to simplify SELECT functions
*/  
std::string SQLQuerier::exclude_asn_string() {
    if (exclude_as_number > -1) {
        return " and monitor_asn != " + std::to_string(exclude_as_number) + ";";
    } else {
        return ";";
    }
}

// Returns a string with a DROP TABLE query
std::string SQLQuerier::clear_table_string(std::string table_name) {
    return "DROP TABLE IF EXISTS " + table_name + ";";
}

/** Returns a string with SQL CREATE query
 *
 *  @param table_name The name of the table to create
 *  @param column_names Comma separated list of colums to be created,
 *                      surrounded by parentheses, Ex: (stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint)
 *  @param unlogged Specifies whether the table is unlogged
 *  @param grant_all_user User to grant privileges to,
 *                        if left empty GRANT ALL won't be included in the query
 */
std::string SQLQuerier::create_table_string(std::string table_name, std::string column_names, bool unlogged, std::string grant_all_user) {
    std::string unlogged_string = unlogged ? " UNLOGGED " : " ";
    std::string grant_all_string = (grant_all_user == "") ? "" : " GRANT ALL ON TABLE " + table_name + " TO " + grant_all_user + ";";

    std::string sql = "CREATE" + unlogged_string + "TABLE IF NOT EXISTS " + table_name +
    + " " + column_names + ";" + grant_all_string;

    return sql;
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
    std::string sql = select_prefix_string(p);

    BOOST_LOG_TRIVIAL(info) << "Select prefix count query: " << sql;

    return execute(sql);
}


/** Pulls all announcements for the announcements for the given prefix.
 *
 * @param p The prefix for which we SELECT
 */
pqxx::result SQLQuerier::select_prefix_ann(Prefix<>* p) {
    std::string sql = select_prefix_string(p, false, "host(prefix), netmask(prefix), as_path, origin, time");

    BOOST_LOG_TRIVIAL(info) << "Select prefix ann query: " << sql;

    return execute(sql);
}


/** Pulls the count for all announcements for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
pqxx::result SQLQuerier::select_subnet_count(Prefix<>* p) {
    std::string sql = select_prefix_string(p, true);

    BOOST_LOG_TRIVIAL(info) << "Select subnet count query: " << sql;

    return execute(sql);
}


/** Pulls all announcements for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
pqxx::result SQLQuerier::select_subnet_ann(Prefix<>* p) {
    std::string sql = select_prefix_string(p, true, "host(prefix), netmask(prefix), as_path, origin, time");

    BOOST_LOG_TRIVIAL(info) << "Select subnet ann query: " << sql;

    return execute(sql);
}


/** Drops the stubs table
 */
void SQLQuerier::clear_stubs_from_db() {
    std::string sql = clear_table_string(STUBS_TABLE);

    BOOST_LOG_TRIVIAL(info) << "Clear stubs query: " << sql;

    //execute(sql);
}


/** Drops the non stubs table
 */
void SQLQuerier::clear_non_stubs_from_db() {
    std::string sql = clear_table_string(NON_STUBS_TABLE);

    BOOST_LOG_TRIVIAL(info) << "Clear non stubs query: " << sql;

    //execute(sql);
}


/** Drops the supernodes table
 */
void SQLQuerier::clear_supernodes_from_db() {
    std::string sql = clear_table_string(SUPERNODES_TABLE);

    BOOST_LOG_TRIVIAL(info) << "Clear supernodes query: " << sql;

    //execute(sql);
}


/** Instantiates a new, empty stubs table in the database, if it doesn't exist.
 */
void SQLQuerier::create_stubs_tbl() {
    // std::string sql = std::string("CREATE TABLE IF NOT EXISTS " STUBS_TABLE " (stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint);");
    // BOOST_LOG_TRIVIAL(info) << "Creating stubs table...";
    // execute(sql, false);

    std::string sql = create_table_string(STUBS_TABLE, "(stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint)");
    BOOST_LOG_TRIVIAL(info) << "Creating stubs table...";
    BOOST_LOG_TRIVIAL(info) << "Query: " << sql;
    //execute(sql, false);
}


/** Instantiates a new, empty non_stubs table in the database, if it doesn't exist.
 */
void SQLQuerier::create_non_stubs_tbl() {
    // std::string sql = std::string("CREATE TABLE IF NOT EXISTS " NON_STUBS_TABLE " (non_stub_asn BIGSERIAL PRIMARY KEY);");
    // BOOST_LOG_TRIVIAL(info) << "Creating non_stubs table...";
    // execute(sql, false);

    std::string sql = create_table_string(NON_STUBS_TABLE, "(non_stub_asn BIGSERIAL PRIMARY KEY)");
    BOOST_LOG_TRIVIAL(info) << "Creating non_stubs table...";
    BOOST_LOG_TRIVIAL(info) << "Query: " << sql;
    //execute(sql, false);
}


/**  Instantiates a new, empty supernodes table in the database, if it doesn't exist.
 */
void SQLQuerier::create_supernodes_tbl() {
    // std::string sql = std::string("CREATE TABLE IF NOT EXISTS " SUPERNODES_TABLE "(supernode_asn BIGSERIAL PRIMARY KEY, supernode_lowest_asn bigint)");
    // BOOST_LOG_TRIVIAL(info) << "Creating supernodes table...";
    // execute(sql, false);

    std::string sql = create_table_string(SUPERNODES_TABLE, "(supernode_asn BIGSERIAL PRIMARY KEY, supernode_lowest_asn bigint)");
    BOOST_LOG_TRIVIAL(info) << "Creating supernodes table...";
    BOOST_LOG_TRIVIAL(info) << "Query: " << sql;
    //execute(sql, false;)
}


/** Takes a .csv filename and bulk copies all elements to the stubs table.
 */
void SQLQuerier::copy_stubs_to_db(std::string file_name) {
    //std::string sql = "COPY " STUBS_TABLE "(stub_asn,parent_asn) FROM '" +
    //                  file_name + "' WITH (FORMAT csv)";


    std::string sql = copy_to_db_string(file_name, STUBS_TABLE, "(stub_asn,parent_asn)");

    BOOST_LOG_TRIVIAL(info) << "Copy stubs to db query: " << sql;

    //execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the non-stubs table.
 */
void SQLQuerier::copy_non_stubs_to_db(std::string file_name) {
    //std::string sql = "COPY " NON_STUBS_TABLE "(non_stub_asn) FROM '" +
    //                  file_name + "' WITH (FORMAT csv)";

    std::string sql = copy_to_db_string(file_name, NON_STUBS_TABLE, "(non_stub_asn)");

    BOOST_LOG_TRIVIAL(info) << "Copy non stubs to db query: " << sql;

    //execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the supernodes table.
 */
void SQLQuerier::copy_supernodes_to_db(std::string file_name) {
    //std::string sql = "COPY " SUPERNODES_TABLE "(supernode_asn,supernode_lowest_asn) FROM '" +
    //                  file_name + "' WITH (FORMAT csv)";

    std::string sql = copy_to_db_string(file_name, SUPERNODES_TABLE, "(supernode_asn,supernode_lowest_asn)");

    BOOST_LOG_TRIVIAL(info) << "Copy supernodes to db query: " << sql;

    //execute(sql);
}


/** Drop the Querier's results table.
 */
void SQLQuerier::clear_results_from_db() {
    std::string sql = clear_table_string(results_table);

    BOOST_LOG_TRIVIAL(info) << "Clear results from db query: " << sql;

    //execute(sql);
}


/** Drop the Querier's depref table.
 */
void SQLQuerier::clear_depref_from_db() {
    std::string sql = clear_table_string(depref_table);

    BOOST_LOG_TRIVIAL(info) << "Clear depref from db query: " << sql;

    //execute(sql);
}


/** Drop the Querier's inverse results table.
 */
void SQLQuerier::clear_inverse_from_db() {
    std::string sql = clear_table_string(inverse_results_table);

    BOOST_LOG_TRIVIAL(info) << "Clear inverse from db query: " << sql;

    //execute(sql);
}


/** Instantiates a new, empty results table in the database, dropping the old table.
 */
void SQLQuerier::create_results_tbl() {
    /**
    std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + results_table + " (ann_id serial PRIMARY KEY,\
    */
    // std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + results_table + " (\
    // asn bigint,prefix cidr, origin bigint, received_from_asn \
    // bigint, time bigint); GRANT ALL ON TABLE " + results_table + " TO bgp_user;");
    // BOOST_LOG_TRIVIAL(info) << "Creating results table...";
    // execute(sql, false);

    std::string sql = create_table_string(results_table, "(asn bigint,prefix cidr, origin bigint, received_from_asn bigint, time bigint)",
    true, "bgp_user");

    BOOST_LOG_TRIVIAL(info) << "Creating results table...";
    BOOST_LOG_TRIVIAL(info) << "Query: " << sql;

    //execute(sql, false);
}


/** Instantiates a new, empty depref table in the database, dropping the old table.
 */
void SQLQuerier::create_depref_tbl() {
    // std::string sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS " + depref_table + " (\
    // asn bigint,prefix cidr, origin bigint, received_from_asn \
    // bigint, time bigint); GRANT ALL ON TABLE " + depref_table + " TO bgp_user;");
    // BOOST_LOG_TRIVIAL(info) << "Creating depref table...";
    // execute(sql, false);

    std::string sql = create_table_string(depref_table, "(asn bigint,prefix cidr, origin bigint, received_from_asn bigint, time bigint)",
    true, "bgp_user");

    BOOST_LOG_TRIVIAL(info) << "Creating depref table...";
    BOOST_LOG_TRIVIAL(info) << "Query: " << sql;

    //execute(sql, false);
}


/** Instantiates a new, empty inverse results table in the database, dropping the old table.
 */
void SQLQuerier::create_inverse_results_tbl() {
    // std::string sql;
    // sql = std::string("CREATE UNLOGGED TABLE IF NOT EXISTS ") + inverse_results_table + 
    // "(asn bigint,prefix cidr, origin bigint) ";
    // sql += ";";
    // sql += "GRANT ALL ON TABLE " + inverse_results_table + " TO bgp_user;";
    // BOOST_LOG_TRIVIAL(info) << "Creating inverse results table...";
    // execute(sql, false);

    std::string sql = create_table_string(inverse_results_table, "(asn bigint,prefix cidr, origin bigint)",
    true, "bgp_user");

    BOOST_LOG_TRIVIAL(info) << "Creating inverse results table...";
    BOOST_LOG_TRIVIAL(info) << "Query: " << sql;

    //execute(sql, false);
}


/** Takes a .csv filename and bulk copies all elements to the results table.
 */
void SQLQuerier::copy_results_to_db(std::string file_name) {
    //std::string sql = std::string("COPY " + results_table + "(asn, prefix, origin, received_from_asn, time)") +
    //                              "FROM '" + file_name + "' WITH (FORMAT csv)";

    std::string sql = copy_to_db_string(file_name, results_table, "(asn, prefix, origin, received_from_asn, time)");

    BOOST_LOG_TRIVIAL(info) << "Copy results to db query: " << sql;

    //execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the depref table.
 */
void SQLQuerier::copy_depref_to_db(std::string file_name) {
    //std::string sql = std::string("COPY " + depref_table + "(asn, prefix, origin, received_from_asn, time)") +
    //                              "FROM '" + file_name + "' WITH (FORMAT csv)";

    std::string sql = copy_to_db_string(file_name, depref_table, "(asn, prefix, origin, received_from_asn, time)");

    BOOST_LOG_TRIVIAL(info) << "Copy depref to db query: " << sql;

    //execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the inverse results table.
 */
void SQLQuerier::copy_inverse_results_to_db(std::string file_name) {
    //std::string sql = std::string("COPY " + inverse_results_table + "(asn, prefix, origin)") +
    //                              "FROM '" + file_name + "' WITH (FORMAT csv)";

    std::string sql = copy_to_db_string(file_name, inverse_results_table, "(asn, prefix, origin)");

    BOOST_LOG_TRIVIAL(info) << "Copy inverse to db query: " << sql;

    //execute(sql);
}


/** Generate an index on the results table.
 */
void SQLQuerier::create_results_index() {
    // Version of postgres must support this
    std::string sql = std::string("CREATE INDEX ON " + results_table + " USING GIST(prefix inet_ops, origin)");
    BOOST_LOG_TRIVIAL(info) << "Generating index on results...";
    execute(sql, false);
}
