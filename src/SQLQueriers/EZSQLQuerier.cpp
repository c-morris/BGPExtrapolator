#include "SQLQueriers/EZSQLQuerier.h"

EZSQLQuerier::EZSQLQuerier(std::string a, std::string r, std::string i, std::string d, std::vector<std::string> *pt) : SQLQuerier(a, r, i, d) {
   if (pt != NULL)
       this->policy_tables = *pt;
}

EZSQLQuerier::~EZSQLQuerier() {
    
}

pqxx::result EZSQLQuerier::select_prefix_ann(Prefix<>* p) {
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin, time, origin_hijack_asn FROM " + announcements_table;
    sql += " WHERE prefix = \'" + cidr + "\';";
    return execute(sql);
}

pqxx::result EZSQLQuerier::select_subnet_ann(Prefix<>* p) {
    std::string cidr = p->to_cidr();
    std::string sql = "SELECT host(prefix), netmask(prefix), as_path, origin, time, origin_hijack_asn FROM " + announcements_table;
    sql += " WHERE prefix <<= \'" + cidr + "\';";
    return execute(sql);
}

pqxx::result EZSQLQuerier::select_policy_assignments(std::string const& policy_table){
    std::string sql = "SELECT asn, as_type, impliment FROM " + policy_table + ";";
    return execute(sql);
}
