#include "SQLQueriers/EZSQLQuerier.h" 

EZSQLQuerier::EZSQLQuerier(std::string a, std::string r, std::string i, std::string d, std::string f, std::vector<std::string> *pt, int n, std::string s) : SQLQuerier<>(a, r, i, d, f, n, s) {
   if (pt != NULL)
       this->policy_tables = *pt;
}

EZSQLQuerier::~EZSQLQuerier() {
    
}

pqxx::result EZSQLQuerier::get_attacker_po_pairs() {
    std::string sql = "SELECT DISTINCT host(prefix), netmask(prefix), origin_hijack_asn, prefix_id, prefix_block_id FROM " + announcements_table;
    sql += " WHERE origin_hijack_asn IS NOT NULL;";
    return execute(sql);
}

pqxx::result EZSQLQuerier::select_policy_assignments(std::string const& policy_table){
    std::string sql = "SELECT asn, as_type, impliment FROM " + policy_table + ";";
    return execute(sql);
}

void EZSQLQuerier::create_round_results_tbl(int i) {
    std::stringstream sql;
    sql << "CREATE UNLOGGED TABLE IF NOT EXISTS " << EZBGPSEC_ROUND_TABLE_BASE_NAME <<
    i << " (asn bigint,prefix cidr, origin bigint, received_from_asn bigint, time bigint, prefix_id bigint);" <<
    "GRANT ALL ON TABLE " << EZBGPSEC_ROUND_TABLE_BASE_NAME << i << " TO bgp_user;";

    BOOST_LOG_TRIVIAL(info) << "Creating round " << i << " results table...";
    execute(sql.str(), false);
}


void EZSQLQuerier::clear_round_results_from_db(int i) {
    std::stringstream sql;
    sql << "DROP TABLE IF EXISTS " << EZBGPSEC_ROUND_TABLE_BASE_NAME << i << ";";
    execute(sql.str());
}

