#ifndef EZ_AS_H
#define EZ_AS_H

#include "ASes/BaseAS.h"
#include "Announcements/EZAnnouncement.h"

class EZAS : public BaseAS<EZAnnouncement> {
public:
    EZAS(uint32_t asn, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results);
    EZAS(uint32_t asn);
    EZAS();

    ~EZAS();
};

#endif