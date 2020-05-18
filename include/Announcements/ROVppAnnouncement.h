#ifndef ROV_ANNOUNCEMENT_H
#define ROV_ANNOUNCEMENT_H

#include "Announcements/Announcement.h"

class ROVppAnnouncement : public Announcement {
public:
    uint32_t alt;               // flag meaning a "hole" along the path
    uint32_t tiebreak_override; // ensure tiebreaks propagate where they should
    uint32_t sent_to_asn;       // ASN this ann is being sent to

    bool withdraw;              // if this is a withdrawn route
    std::vector<uint32_t> as_path; // stores full as path

    /** Default constructor
     */
    ROVppAnnouncement(uint32_t aorigin, 
                 uint32_t aprefix, 
                 uint32_t anetmask,
                 uint32_t from_asn, 
                 int64_t timestamp = 0);
    
    /** Priority constructor
     */
    ROVppAnnouncement(uint32_t aorigin, 
                 uint32_t aprefix, 
                 uint32_t anetmask,
                 uint32_t pr, 
                 uint32_t from_asn, 
                 int64_t timestamp, 
                 const std::vector<uint32_t> &path,
                 bool a_from_monitor = false);

    ROVppAnnouncement(uint32_t aorigin, 
                 uint32_t aprefix, 
                 uint32_t anetmask,
                 uint32_t pr, 
                 uint32_t from_asn, 
                 int64_t timestamp,
                 uint32_t policy, 
                 const std::vector<uint32_t> &path,
                 bool a_from_monitor = false);

    /** Copy constructor
     */
    ROVppAnnouncement(const ROVppAnnouncement& ann);

    /** Copy assignment
     */
    ROVppAnnouncement& operator=(ROVppAnnouncement ann);

    /** Swap
     */
    friend void swap(ROVppAnnouncement& a, ROVppAnnouncement& b);

    /** Defines the << operator for the Announcements
     *
     * For use in debugging, this operator prints an announcements to an output stream.
     * 
     * @param &os Specifies the output stream.
     * @param ann Specifies the announcement from which data is pulled.
     * @return The output stream parameter for reuse/recursion.
     */ 
    friend std::ostream& operator<<(std::ostream &os, const ROVppAnnouncement& ann);

    /** Passes the announcement struct data to an output stream to csv generation.
     *
     * @param &os Specifies the output stream.
     * @return The output stream parameter for reuse/recursion.
     */ 
    virtual std::ostream& to_csv(std::ostream &os);

    /** Passes the announcement struct data to an output stream to csv generation.
     * For creating the rovpp_blackholes table only.
     * 
     * @param &os Specifies the output stream.
     * @return The output stream parameter for reuse/recursion.
     */ 
    virtual std::ostream& to_blackholes_csv(std::ostream &os);
    
    bool operator==(const ROVppAnnouncement &b) const;
    bool operator!=(const ROVppAnnouncement &b) const;
    bool operator<(const ROVppAnnouncement &b) const;
};
#endif