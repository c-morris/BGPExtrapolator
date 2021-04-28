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

#ifndef ROVPP_ANNOUNCEMENT_H
#define ROVPP_ANNOUNCEMENT_H

#include "Announcements/Announcement.h"

class ROVppAnnouncement : public Announcement {
public:
    uint32_t alt;               // flag meaning a "hole" along the path
    uint32_t tiebreak_override; // ensure tiebreaks propagate where they should
    uint32_t sent_to_asn;       // ASN this ann is being sent to
    uint32_t priority;          // Announcement uses the Priority struct now, but the ROVpp version still uses uint32 instead

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