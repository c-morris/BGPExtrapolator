#include "SQLQueriers/ROVSQLQuerier.h"

ROVSQLQuerier::ROVSQLQuerier(std::vector<std::string> policy_tables, 
                    std::string announcements_table, 
                    std::string results_table,
                    std::string full_path_results_table,
                    int exclude_as_number,
                    std::string config_section) 
: SQLQuerier(announcements_table, results_table, INVERSE_RESULTS_TABLE, DEPREF_RESULTS_TABLE, full_path_results_table, exclude_as_number, config_section) {
    this->policy_tables = policy_tables;
}

ROVSQLQuerier::~ROVSQLQuerier() { }

/** Pulls all ASes and policies they implement.
 *
 * @param policy_table The table name holding the list of ASNs with their policies, default is 'edge_ases'
 */
pqxx::result ROVSQLQuerier::select_AS_flags(std::string const& policy_table){
    std::string sql = "SELECT asn, as_type, impliment FROM " + policy_table + ";";
    return execute(sql);
}
