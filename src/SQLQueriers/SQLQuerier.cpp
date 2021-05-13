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

template <typename PrefixType>
SQLQuerier<PrefixType>::SQLQuerier(std::string announcements_table /* = ANNOUNCEMENTS_TABLE */,
                        std::string results_table /* = RESULTS_TABLE */, 
                        std::string inverse_results_table /* = INVERSE_RESULTS_TABLE */, 
                        std::string depref_results_table /* = DEPREF_RESULTS_TABLE */,
                        std::string full_path_results_table /* = FULL_PATH_RESULTS_TABLE */,
                        int exclude_as_number, /* = -1 */
                        std::string config_section /* = "bgp" */,
                        std::string config_path /* = "/etc/bgp/bgp.conf" */,
                        bool create_connection /* = true */) {
    this->announcements_table = announcements_table;
    this->results_table = results_table;
    this->depref_table = depref_results_table;
    this->inverse_results_table = inverse_results_table;
    this->full_path_results_table = full_path_results_table;
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

template <typename PrefixType>
SQLQuerier<PrefixType>::~SQLQuerier() {
    C->disconnect();
    delete C;
}


/** Reads credentials/connection info from .conf file
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::read_config() {
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
template <typename PrefixType>
void SQLQuerier<PrefixType>::open_connection() {
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
template <typename PrefixType>
void SQLQuerier<PrefixType>::close_connection() {
    C->disconnect();
}


/** Executes a give SQL statement
 *
 *  @param sql
 *  @param insert
 */
template <typename PrefixType>
pqxx::result SQLQuerier<PrefixType>::execute(std::string sql, bool insert) {
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
template <typename PrefixType>
std::string SQLQuerier<PrefixType>::copy_to_db_query_string(std::string file_name, std::string table_name, std::string column_names) {
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
 *                   Ex: host(prefix), netmask(prefix), as_path, origin, time
 *                   Replaced by COUNT(*) if unspecified
 */
template <typename PrefixType>
std::string SQLQuerier<PrefixType>::select_prefix_query_string(Prefix<PrefixType>* p, bool subnet, std::string selection) {
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT " + selection + " FROM " + announcements_table;
    if (subnet) {
        sql += " WHERE prefix <<= \'" + cidr + "\'";
    } else {
        sql += " WHERE prefix = \'" + cidr + "\'";
    }

    if (exclude_as_number > -1) {
        sql += " and monitor_asn != " + std::to_string(exclude_as_number) + ";";
    } else {
        sql += ";";
    }

    return sql;
}

// Returns a string with a DROP TABLE query
template <typename PrefixType>
std::string SQLQuerier<PrefixType>::clear_table_query_string(std::string table_name) {
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
template <typename PrefixType>
std::string SQLQuerier<PrefixType>::create_table_query_string(std::string table_name, std::string column_names, bool unlogged, std::string grant_all_user) {
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
template <typename PrefixType>
pqxx::result SQLQuerier<PrefixType>::select_from_table(std::string table_name, int limit) {
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
template <typename PrefixType>
pqxx::result SQLQuerier<PrefixType>::select_prefix_count(Prefix<PrefixType>* p) {
    std::string sql = select_prefix_query_string(p);
    return execute(sql);
}


/** Pulls all announcements for the announcements for the given prefix.
 *
 * @param p The prefix for which we SELECT
 */
template <typename PrefixType>
pqxx::result SQLQuerier<PrefixType>::select_prefix_ann(Prefix<PrefixType>* p) {
    std::string sql = select_prefix_query_string(p, false, "host(prefix), netmask(prefix), as_path, origin, time, prefix_id");
    return execute(sql);
}


/** Pulls the count for all announcements for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
template <typename PrefixType>
pqxx::result SQLQuerier<PrefixType>::select_subnet_count(Prefix<PrefixType>* p) {
    std::string sql = select_prefix_query_string(p, true);
    return execute(sql);
}


/** Pulls all announcements for the prefixes contained within the passed subnet.
 *
 * @param p The prefix defining the subnet
 */
template <typename PrefixType>
pqxx::result SQLQuerier<PrefixType>::select_subnet_ann(Prefix<PrefixType>* p) {
    std::string sql = select_prefix_query_string(p, true, "host(prefix), netmask(prefix), as_path, origin, time, prefix_id");
    return execute(sql);
}


/** Drops the stubs table
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::clear_stubs_from_db() {
    std::string sql = clear_table_query_string(STUBS_TABLE);
    execute(sql);
}


/** Drops the non stubs table
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::clear_non_stubs_from_db() {
    std::string sql = clear_table_query_string(NON_STUBS_TABLE);
    execute(sql);
}


/** Drops the supernodes table
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::clear_supernodes_from_db() {
    std::string sql = clear_table_query_string(SUPERNODES_TABLE);
    execute(sql);
}


/** Instantiates a new, empty stubs table in the database, if it doesn't exist.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::create_stubs_tbl() {
    std::string sql = create_table_query_string(STUBS_TABLE, "(stub_asn BIGSERIAL PRIMARY KEY,parent_asn bigint)");
    BOOST_LOG_TRIVIAL(info) << "Creating stubs table...";
    execute(sql, false);
}


/** Instantiates a new, empty non_stubs table in the database, if it doesn't exist.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::create_non_stubs_tbl() {
    std::string sql = create_table_query_string(NON_STUBS_TABLE, "(non_stub_asn BIGSERIAL PRIMARY KEY)");
    BOOST_LOG_TRIVIAL(info) << "Creating non_stubs table...";
    execute(sql, false);
}


/**  Instantiates a new, empty supernodes table in the database, if it doesn't exist.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::create_supernodes_tbl() {
    std::string sql = create_table_query_string(SUPERNODES_TABLE, "(supernode_asn BIGSERIAL PRIMARY KEY, supernode_lowest_asn bigint)");
    BOOST_LOG_TRIVIAL(info) << "Creating supernodes table...";
    execute(sql, false);
}


/** Takes a .csv filename and bulk copies all elements to the stubs table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::copy_stubs_to_db(std::string file_name) {
    std::string sql = copy_to_db_query_string(file_name, STUBS_TABLE, "(stub_asn,parent_asn)");
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the non-stubs table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::copy_non_stubs_to_db(std::string file_name) {
    std::string sql = copy_to_db_query_string(file_name, NON_STUBS_TABLE, "(non_stub_asn)");
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the supernodes table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::copy_supernodes_to_db(std::string file_name) {
    std::string sql = copy_to_db_query_string(file_name, SUPERNODES_TABLE, "(supernode_asn,supernode_lowest_asn)");
    execute(sql);
}


/** Drop the Querier's results table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::clear_results_from_db() {
    std::string sql = clear_table_query_string(results_table);
    execute(sql);
}


/** Drop the Querier's depref table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::clear_depref_from_db() {
    std::string sql = clear_table_query_string(depref_table);
    execute(sql);
}


/** Drop the Querier's inverse results table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::clear_inverse_from_db() {
    std::string sql = clear_table_query_string(inverse_results_table);
    execute(sql);
}

/** Drop the Querier's full path results table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::clear_full_path_from_db() {
    std::string sql = clear_table_query_string(full_path_results_table);
    execute(sql);
}

/** Instantiates a new, empty results table in the database, dropping the old table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::create_results_tbl() {
    std::string sql = create_table_query_string(results_table, "(asn bigint, prefix inet, origin bigint, received_from_asn bigint, time bigint, prefix_id bigint)",
    true, user);
    BOOST_LOG_TRIVIAL(info) << "Creating results table...";
    execute(sql, false);
}

/** Instantiates a new, empty full path results table in the database, dropping the old table.
 *
 * In addition to all of the columns in the results table, this table includes the as_path.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::create_full_path_results_tbl() {
    std::string sql = create_table_query_string(full_path_results_table, 
    "(asn bigint, prefix inet, origin bigint, received_from_asn bigint, time bigint, prefix_id bigint, as_path bigint[])", true, user);
    BOOST_LOG_TRIVIAL(info) << "Creating full path results table...";
    execute(sql, false);
}

/** Instantiates a new, empty depref table in the database, dropping the old table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::create_depref_tbl() {
    std::string sql = create_table_query_string(depref_table, "(asn bigint, prefix inet, origin bigint, received_from_asn bigint, time bigint)",
    true, user);
    BOOST_LOG_TRIVIAL(info) << "Creating depref table...";
    execute(sql, false);
}


/** Instantiates a new, empty inverse results table in the database, dropping the old table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::create_inverse_results_tbl() {
    std::string sql = create_table_query_string(inverse_results_table, "(asn bigint, prefix inet, origin bigint)",
    true, user);
    BOOST_LOG_TRIVIAL(info) << "Creating inverse results table...";
    execute(sql, false);
}


/** Takes a .csv filename and bulk copies all elements to the results table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::copy_results_to_db(std::string file_name) {
    std::string sql = copy_to_db_query_string(file_name, results_table, "(asn, prefix, origin, received_from_asn, time, prefix_id)");
    execute(sql);
}

/** Similar to copy_results_to_db, but for a single AS result which includes an AS_PATH column.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::copy_single_results_to_db(std::string file_name) {
    std::string sql = copy_to_db_query_string(file_name, full_path_results_table, "(asn, prefix, origin, received_from_asn, time, prefix_id, as_path)");
    execute(sql);
}

/** Takes a .csv filename and bulk copies all elements to the depref table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::copy_depref_to_db(std::string file_name) {
    std::string sql = copy_to_db_query_string(file_name, depref_table, "(asn, prefix, origin, received_from_asn, time)");
    execute(sql);
}


/** Takes a .csv filename and bulk copies all elements to the inverse results table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::copy_inverse_results_to_db(std::string file_name) {
    std::string sql = copy_to_db_query_string(file_name, inverse_results_table, "(asn, prefix, origin)");
    execute(sql);
}


/** Generate an index on the results table.
 */
template <typename PrefixType>
void SQLQuerier<PrefixType>::create_results_index() {
    // Version of postgres must support this
    std::string sql = std::string("CREATE INDEX ON " + results_table + " USING GIST(prefix inet_ops, origin)");
    BOOST_LOG_TRIVIAL(info) << "Generating index on results...";
    execute(sql, false);
}

/** Returns the max value of block_id in the announcements table
 */
template <typename PrefixType>
pqxx::result SQLQuerier<PrefixType>::select_max_block_id() {
    std::string sql = std::string("SELECT MAX(block_id) FROM " + announcements_table + ";");
    return execute(sql, false);
}

/** Returns all rows (announcements) that have the corresponding block_id
 */
template <typename PrefixType>
pqxx::result SQLQuerier<PrefixType>::select_prefix_block_id(int block_id, int family) {
    std::string sql = std::string("SELECT host(prefix), netmask(prefix), as_path, origin, time, prefix_id FROM "
     + announcements_table + " WHERE block_id = " + std::to_string(block_id)
     + " AND family(prefix) = " + std::to_string(family));

    if (exclude_as_number > -1) {
        sql += " and monitor_asn != " + std::to_string(exclude_as_number) + ";";
    } else {
        sql += ";";
    }

    return execute(sql, false);
}

template class SQLQuerier<>;
template class SQLQuerier<uint128_t>;
