#ifndef EZ_SQL_QUERIER_H
#define EZ_SQL_QUERIER_H

#include <sstream>

#include "SQLQueriers/SQLQuerier.h"

class EZSQLQuerier : public SQLQuerier {
public:
    std::vector<std::string> policy_tables;

    EZSQLQuerier(std::string a=ANNOUNCEMENTS_TABLE, 
                    std::string r=RESULTS_TABLE,
                    std::string i=INVERSE_RESULTS_TABLE,
                    std::string d=DEPREF_RESULTS_TABLE,
                    std::vector<std::string> *pt=NULL,
                    int n=-1,
                    std::string s=DEFAULT_QUERIER_CONFIG_SECTION);
    ~EZSQLQuerier();

    pqxx::result select_subnet_ann(Prefix<>* p); 
    pqxx::result select_prefix_ann(Prefix<>* p);

    /** Loads AS policy assignments from the database.
     *
     * Like select_AS_flags in the ROVppSQLQuerier, but with a more descriptive name.
     *
     * @param policy_table name of the table holding the policy assignments
     */
    pqxx::result select_policy_assignments(std::string const& policy_table);

    /** Instantiates a new, empty round results table in the database.
     *
     * The schema is identical to a regular results table. 
     */
    void create_round_results_tbl(int i);

    void clear_round_results_from_db(int i);

};

#endif
