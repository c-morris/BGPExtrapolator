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

pqxx::result ROVSQLQuerier::select_AS_flags(std::string const& policy_table){
    std::string sql = "SELECT asn, as_type, impliment FROM " + policy_table + ";";
    return execute(sql);
}

pqxx::result ROVSQLQuerier::select_prefix_ann(Prefix<>* p) {
    std::string sql = select_prefix_query_string(p, false, "host(prefix), netmask(prefix), as_path, origin, time, prefix_id, block_prefix_id, roa_validity");
    return execute(sql);
}

pqxx::result ROVSQLQuerier::select_subnet_ann(Prefix<>* p) {
    std::string sql = select_prefix_query_string(p, true, "host(prefix), netmask(prefix), as_path, origin, time, prefix_id, block_prefix_id, roa_validity");
    return execute(sql);
}
