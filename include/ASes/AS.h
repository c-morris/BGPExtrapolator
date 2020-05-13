#ifndef AS_H
#define AS_H

#include "ASes/BaseAS.h"

class AS : public BaseAS<Announcement> {
public:
    AS(uint32_t myasn=0, 
        std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results=NULL,
        std::set<uint32_t> *prov=NULL, 
        std::set<uint32_t> *peer=NULL,
        std::set<uint32_t> *cust=NULL) : BaseAS(myasn, inverse_results, prov, peer, cust) {

    }
};

#endif