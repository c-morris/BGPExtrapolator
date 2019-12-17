#ifndef ROVPPANNOUNCEMENT_H
#define ROVPPANNOUNCEMENT_H

#include "Announcement.h"

struct ROVppAnnouncement : public Announcement {
    uint32_t policy_index;

    ROVppAnnouncement(uint32_t aorigin,
                      uint32_t aprefix, 
                      uint32_t anetmask,
                      uint32_t from_asn,
                      int64_t timestamp = 0, 
                      uint32_t policy = 0)
                      : Announcement(aorigin, 
                                     aprefix, 
                                     anetmask, 
                                     from_asn, 
                                     timestamp) {
        policy_index = policy;
    }

    ROVppAnnouncement(uint32_t aorigin, 
                      uint32_t aprefix, 
                      uint32_t anetmask,
                      uint32_t pr, 
                      uint32_t from_asn, 
                      int64_t timestamp,
                      uint32_t policy,
                      bool a_from_monitor = false)
                      : Announcement(aorigin,
                                     aprefix,
                                     anetmask,
                                     pr,
                                     from_asn,
                                     timestamp,
                                     a_from_monitor) {
        policy_index = policy;
    }

    ~ROVppAnnouncement() { }
};

#endif
