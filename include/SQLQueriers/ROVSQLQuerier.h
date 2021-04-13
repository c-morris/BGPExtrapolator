#ifndef ROV_SQL_QUERIER_H
#define ROV_SQL_QUERIER_H

#include "SQLQueriers/SQLQuerier.h"

class ROVSQLQuerier : public SQLQuerier {
public:
    std::vector<std::string> policy_tables;

    ROVSQLQuerier(std::vector<std::string> policy_tables = std::vector<std::string>(),
                    std::string announcements_table = ROV_ANNOUNCEMENTS_TABLE, 
                    std::string results_table = SIMULATION_RESULTS_TABLE,
                    std::string full_path_results_table = FULL_PATH_RESULTS_TABLE,
                    int exclude_as_number = -1,
                    std::string config_section = DEFAULT_QUERIER_CONFIG_SECTION);
    ~ROVSQLQuerier();

    pqxx::result select_AS_flags(std::string const& policy_table = std::string("edge_ases"));
    pqxx::result select_prefix_ann(Prefix<>* p);
    pqxx::result select_subnet_ann(Prefix<>* p);
};

#endif
