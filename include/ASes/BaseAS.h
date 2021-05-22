/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#ifndef BASE_AS_H
#define BASE_AS_H

// Define relationship macros
#define AS_REL_PROVIDER 0
#define AS_REL_PEER 1
#define AS_REL_CUSTOMER 2
// Keep old values for ROVpp
#define AS_REL_PROVIDER_ROVPP 0
#define AS_REL_PEER_ROVPP 100
#define AS_REL_CUSTOMER_ROVPP 200

#include <type_traits>
#include <algorithm>  
#include <string>
#include <set>
#include <map>
#include <vector>
#include <random>
#include <iostream>

#include "Prefix.h"
#include "PrefixAnnouncementMap.h"

#include "Announcements/Announcement.h"
#include "Announcements/EZAnnouncement.h"
#include "Announcements/ROVppAnnouncement.h"
#include "Announcements/ROVAnnouncement.h"

template <class AnnouncementType, typename PrefixType = uint32_t>
class BaseAS {

//Ensure that the Announcement class passed in through the template extends the Announcement class.
static_assert(std::is_base_of<Announcement<PrefixType>, AnnouncementType>::value, "AnnouncementType must inherit from Announcement");

public:
    uint32_t asn;       // Autonomous System Number
    bool visited;       // Marks something
    int rank;           // Rank in ASGraph heirarchy for propagation 
    // Defer processing of incoming announcements for efficiency
    std::vector<AnnouncementType> *incoming_announcements;
    // Maps of all announcements stored
    PrefixAnnouncementMap<AnnouncementType, PrefixType> *all_anns;
    PrefixAnnouncementMap<AnnouncementType, PrefixType> *depref_anns;

    // Stores AS Relationships
    std::set<uint32_t> *providers; 
    std::set<uint32_t> *peers; 
    std::set<uint32_t> *customers; 
    // Pointer to inverted results map for efficiency
    std::map<std::pair<Prefix<PrefixType>, uint32_t>,std::set<uint32_t>*> *inverse_results; 
    // If this AS represents multiple ASes, it's "members" are listed here (Supernodes)
    std::vector<uint32_t> *member_ases;
    // Assigned and used in Tarjan's algorithm
    int index;
    int lowlink;
    bool onStack;
    
    // Constructor. Must be in header file.... We like C++ class templates. We like C++ class templates....
    BaseAS(uint32_t asn, uint32_t max_block_prefix_id, bool store_depref_results, std::map<std::pair<Prefix<PrefixType>, uint32_t>, std::set<uint32_t>*> *inverse_results) {

        // Set ASN
        this->asn = asn;
        // Initialize AS to invalid rank
        rank = -1;     
        
        // Create AS relationship sets
        providers = new std::set<uint32_t>();
        peers = new std::set<uint32_t>();
        customers = new std::set<uint32_t>();

        this->inverse_results = inverse_results;    // Inverted results map
        member_ases = new std::vector<uint32_t>();    // Supernode members
        incoming_announcements = new std::vector<AnnouncementType>();

        all_anns = new PrefixAnnouncementMap<AnnouncementType, PrefixType>(max_block_prefix_id);

        if(store_depref_results)
            depref_anns = new PrefixAnnouncementMap<AnnouncementType, PrefixType>(max_block_prefix_id);
        else
            depref_anns = NULL;

        // Tarjan variables
        index = -1;
        onStack = false;
    }

    BaseAS(uint32_t asn, uint32_t max_block_prefix_id, bool store_depref_results) : BaseAS(asn, max_block_prefix_id, store_depref_results, NULL) { }
    BaseAS(uint32_t asn, uint32_t max_block_prefix_id) : BaseAS(asn, max_block_prefix_id, false, NULL) { }
    BaseAS() : BaseAS(0, 20, false, NULL) { }

    virtual ~BaseAS();
    
    /** Tiny galois field hash with a fixed key of 3.
    */
    virtual uint8_t tiny_hash(uint32_t as_number);

    /** Generates a random boolean value.
    */
    virtual bool get_random();

    //****************** Relationship Handling ******************//

    /** Add neighbor AS to the appropriate set in this AS based on the relationship.
     *
     * @param asn ASN of neighbor.
     * @param relationship AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
     */
    virtual void add_neighbor(uint32_t asn, int relationship);

    /** Remove neighbor AS from the appropriate set in this AS based on the relationship.
     *
     * @param asn ASN of neighbor.
     * @param relationship AS_REL_PROVIDER, AS_REL_PEER, or AS_REL_CUSTOMER.
     */
    virtual void remove_neighbor(uint32_t asn, int relationship);

    //****************** Announcement Handling ******************//

    /** Swap a pair of prefix/origins for this AS in the inverse results.
     *
     * @param old The prefix/origin to be inserted
     * @param current The prefix/origin to be removed
     */
    virtual void swap_inverse_result(std::pair<Prefix<PrefixType>, uint32_t> old, 
                                        std::pair<Prefix<PrefixType>, uint32_t> current);

    /** Push the incoming propagated announcements to the incoming_announcements vector.
     *
     * This is NOT called for seeded announcements.
     *
     * @param announcements The announcements to be pushed onto the incoming_announcements vector.
     */
    virtual void receive_announcements(std::vector<AnnouncementType> &announcements);

    /** Processes a single announcement, adding it to the ASes set of announcements if appropriate.
     *
     * Approximates BGP best path selection based on announcement priority.
     * Called by process_announcements and Extrapolator.give_ann_to_as_path()
     * 
     * @param ann The announcement to be processed
     */ 
    virtual void process_announcement(AnnouncementType &ann, bool ran=true);

    /** Iterate through incoming_announcements and keep only the best. 
    */
    virtual void process_announcements(bool ran=true);

    /** Clear all announcement collections. 
    */
    virtual void clear_announcements();

    /** Check if a monitor announcement is already recv'd by this AS. 
     *
     * @param ann Announcement to check for. 
     * @return True if recv'd, false otherwise.
     */
    virtual bool already_received(AnnouncementType &ann);

    /** Deletes given announcement.
    */
    virtual void delete_ann(AnnouncementType &ann);

    /** Deletes the announcement of given prefix.
    */
    virtual void delete_ann(Prefix<PrefixType> &prefix);

    //****************** FILE I/O ******************//

    /** Insertion operator for AS class.
     *
     * @param os
     * @param as
     * @return os passed as parameter
     */
    template <class U>
    friend std::ostream& operator<<(std::ostream &os, const BaseAS<U>& as);

    /** Streams announcements to an output stream in a .csv readable file format.
     *
     * @param os
     * @return output stream into which is passed the .csv row formatted announcements
     */
    virtual std::ostream& stream_announcements(std::ostream &os);

    /** Streams depref announcements to an output stream in a .csv readable file format.
     *
     * @param os
     * @return output stream into which is passed the .csv row formatted announcements
     */
    virtual std::ostream& stream_depref(std::ostream &os);
};
#endif
