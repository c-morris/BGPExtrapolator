#ifndef ROV_AS_H
#define ROV_AS_H

#include "ASes/BaseAS.h"
#include "Announcements/ROVAnnouncement.h"

#define HIJACKED_ASN 64513
#define NOTHIJACKED_ASN 64514
#define ROVPPAS_TYPE_ROV 2 // Flag for the standard ROV policy

class ROVAS : public BaseAS<ROVAnnouncement> {
public:
    ROVAS(uint32_t asn, uint32_t max_block_prefix_id, std::set<uint32_t> *rov_attackers, bool store_depref_results, std::map<std::pair<Prefix<>, uint32_t>, std::set<uint32_t>*> *inverse_results);
    ROVAS(uint32_t asn, uint32_t max_block_prefix_id, std::set<uint32_t> *rov_attackers, bool store_depref_results);
    ROVAS(uint32_t asn, uint32_t max_block_prefix_id, std::set<uint32_t> *rov_attackers);
    ROVAS(uint32_t asn, uint32_t max_block_prefix_id);
    ROVAS();
    ~ROVAS();

    /** Processes a single ROVAnnouncement, adding it to the AS's set of announcements if appropriate
     *
     * Approximates BGP best path selection based on ROVAnnouncement priority
     * Simulates ROV if this AS adopts it
     * Called by process_announcements and ROVExtrapolator.give_ann_to_as_path()
     * 
     * @param &ann the rovannouncement to be processed
     * @param ran flag to enable random tiebreaks
     */ 
    virtual void process_announcement(ROVAnnouncement &ann, bool ran=true);

    /** Checks whether an announcement was sent by an attacker
     * @param &ann the annoucement to check
     */
    bool is_from_attacker(ROVAnnouncement &ann);
    // Checks whether this AS is an attacker
    bool is_attacker();

    /** Sets ROV adoption of the AS
     * @param adopts_rov true if adopts, false if doesn't
     */
    void set_rov_adoption(bool adopts_rov);
    // Returns ROV adoption of the AS
    bool get_rov_adoption();

private: 
    // Set of all known attackers
    std::set<uint32_t> *attackers;
    // Variable that specifies if this AS adopts the ROV policy
    bool adopts_rov;        
};

#endif