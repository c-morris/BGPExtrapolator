#ifndef ROV_SQL_QUERIER_H
#define ROV_SQL_QUERIER_H

#include "SQLQueriers/SQLQuerier.h"

class ROVSQLQuerier : public SQLQuerier {
public:
    std::vector<std::string> policy_tables;

    ROVSQLQuerier(std::vector<std::string> policy_tables = std::vector<std::string>(),
                    std::string a=ANNOUNCEMENTS_TABLE, 
                    std::string r=RESULTS_TABLE,
                    std::string i=INVERSE_RESULTS_TABLE,
                    std::string d=DEPREF_RESULTS_TABLE,
                    std::string f=FULL_PATH_RESULTS_TABLE,
                    int n=-1,
                    std::string s=DEFAULT_QUERIER_CONFIG_SECTION);
    ~ROVSQLQuerier();

    pqxx::result select_AS_flags(std::string const& policy_table = std::string("edge_ases"));
};

#endif
