#ifndef EZ_AS_H
#define EZ_AS_H

#include "ASes/BaseAS.h"
#include "Announcements/EZAnnouncement.h"

class EZAS : public BaseAS<EZAnnouncement> {
public:
    EZAS(uint32_t myasn = 0, 
        std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results = NULL,
        std::set<uint32_t> *prov = NULL, 
        std::set<uint32_t> *peer = NULL,
        std::set<uint32_t> *cust = NULL);

    ~EZAS();
};

#endif