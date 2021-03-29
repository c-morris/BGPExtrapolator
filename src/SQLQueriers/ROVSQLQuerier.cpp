#include "SQLQueriers/ROVSQLQuerier.h"

ROVSQLQuerier::ROVSQLQuerier(std::vector<std::string> policy_tables, std::string a, std::string r, std::string i, std::string d, std::string f, int n, std::string s) 
: SQLQuerier(a, r, i, d, f, n, s) {
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
