#ifndef EZ_SQL_QUERIER_H
#define EZ_SQL_QUERIER_H

#include "SQLQueriers/SQLQuerier.h"

class EZSQLQuerier : public SQLQuerier {
public:
    std::string victim_table;
    std::string attack_table;

    EZSQLQuerier(std::string r=RESULTS_TABLE,
                    std::string e=VICTIM_TABLE,
                    std::string f=ATTACKER_TABLE);
    ~EZSQLQuerier();
};

#endif