#ifndef EZ_SQL_QUERIER_H
#define EZ_SQL_QUERIER_H

#include "SQLQueriers/SQLQuerier.h"

class EZSQLQuerier : public SQLQuerier {
public:
    EZSQLQuerier(std::string a=ANNOUNCEMENTS_TABLE, 
                    std::string r=RESULTS_TABLE,
                    std::string i=INVERSE_RESULTS_TABLE,
                    std::string d=DEPREF_RESULTS_TABLE,
                    std::string s="bgp");
    ~EZSQLQuerier();
};

#endif