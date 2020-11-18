#ifndef ROVPPANNOUNCEMENT_H
#define ROVPPANNOUNCEMENT_H

#include "Announcement.h"

struct ROVppAnnouncement : public Announcement {
    uint32_t policy_index;      // Stores policy instance the ann belongs to
    int32_t opt_flag;           // TODO Remove after automation script is corrected

    ROVppAnnouncement(uint32_t aorigin,
                      uint32_t aprefix, 
                      uint32_t anetmask,
                      uint32_t from_asn,
                      int64_t timestamp = 0, 
                      uint32_t policy = 0,
                      uint32_t roa_validity = 2)
                      : Announcement(aorigin, 
                                     aprefix, 
                                     anetmask, 
                                     from_asn, 
                                     timestamp,
                                     roa_validity) {
        policy_index = policy;
        opt_flag = 0;
    }

    ROVppAnnouncement(uint32_t aorigin, 
                      uint32_t aprefix, 
                      uint32_t anetmask,
                      uint32_t pr, 
                      uint32_t from_asn, 
                      int64_t timestamp,
                      uint32_t policy,
                      uint32_t roa_validity,
                      const std::vector<uint32_t> &path,
                      bool a_from_monitor = false)
                      : Announcement(aorigin,
                                     aprefix,
                                     anetmask,
                                     pr,
                                     from_asn,
                                     timestamp,
                                     roa_validity,
                                     path,
                                     a_from_monitor) {
        policy_index = policy;
        opt_flag = 0;
    }

    ~ROVppAnnouncement() { }

    /** Passes the announcement struct data to an output stream to csv generation.
     *
     * @param &os Specifies the output stream.
     * @return The output stream parameter for reuse/recursion.
     */ 
    std::ostream& to_csv(std::ostream &os){
        os << prefix.to_cidr() << ',' << origin << ',' << received_from_asn << ',' << tstamp << ',' << opt_flag << ',' << roa_validity << '\n';
        return os;
    }
};

#endif
