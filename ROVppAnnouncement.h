#ifndef ROVPPANNOUNCEMENT_H
#define ROVPPANNOUNCEMENT_H

#include "Announcement.h"

struct ROVppAnnouncement : public Announcement {
    uint32_t policy_index;
    int32_t opt_flag;

    ROVppAnnouncement(uint32_t aorigin,
                      uint32_t aprefix, 
                      uint32_t anetmask,
                      uint32_t from_asn,
                      int64_t timestamp = 0, 
                      uint32_t policy = 0,
                      int32_t  opt = 0)
                      : Announcement(aorigin, 
                                     aprefix, 
                                     anetmask, 
                                     from_asn, 
                                     timestamp) {
        policy_index = policy;
        opt_flag = opt;
    }

    ROVppAnnouncement(uint32_t aorigin, 
                      uint32_t aprefix, 
                      uint32_t anetmask,
                      uint32_t pr, 
                      uint32_t from_asn, 
                      int64_t timestamp,
                      uint32_t policy,
                      bool a_from_monitor = false,
                      int32_t  opt = 0)
                      : Announcement(aorigin,
                                     aprefix,
                                     anetmask,
                                     pr,
                                     from_asn,
                                     timestamp,
                                     a_from_monitor) {
        policy_index = policy;
        opt_flag = opt;
    }

    ~ROVppAnnouncement() { }

    /** Passes the announcement struct data to an output stream to csv generation.
     *
     * @param &os Specifies the output stream.
     * @return The output stream parameter for reuse/recursion.
     */ 
    std::ostream& to_csv(std::ostream &os){
        os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << ',' << opt_flag << '\n';
        return os;
    }
};

#endif
