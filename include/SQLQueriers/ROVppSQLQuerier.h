#ifndef ROVPP_SQLQUERIER_H
#define ROVPP_SQLQUERIER_H

#include "SQLQueriers/SQLQuerier.h"

class ROVppSQLQuerier: public SQLQuerier {
public:
    std::string victim_table;
    std::string attack_table;
    std::vector<std::string> policy_tables;
    
    ROVppSQLQuerier(std::vector<std::string> g,
                    std::string r=RESULTS_TABLE,
                    std::string e=VICTIM_TABLE,
                    std::string f=ATTACKER_TABLE);
    ~ROVppSQLQuerier();

    pqxx::result select_AS_flags(std::string const& flag_table = std::string("rovpp_ases"));
    pqxx::result select_prefix_pairs(Prefix<>* p, std::string const& cur_table);
    pqxx::result select_subnet_pairs(Prefix<>* p, std::string const& cur_table);
    pqxx::result select_all_pairs_from(std::string const& cur_table);
    
    void copy_results_to_db(std::string);
    void create_results_tbl();
    void copy_blackhole_list_to_db(std::string file_name);
    void create_rovpp_blacklist_tbl();
};
#endif