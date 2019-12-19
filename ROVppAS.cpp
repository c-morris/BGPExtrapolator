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

#include "ROVppAS.h"
#include "ROVppAnnouncement.h"

ROVppAS::ROVppAS(uint32_t myasn,
       std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inv,
       std::set<uint32_t> *prov,
       std::set<uint32_t> *peer,
       std::set<uint32_t> *cust) 
    : AS(myasn, inv, prov, peer, cust)  {
    // replace the incoming announcements vector with one of ROVppAnnouncements

    rovpp_incoming_announcements = new std::vector<ROVppAnnouncement>;
    incoming_announcements = rovpp_incoming_announcements;
    rovpp_all_anns = new std::map<Prefix<>, ROVppAnnouncement>;
    all_anns = rovpp_all_anns;
}

ROVppAS::~ROVppAS() { }

void ROVppAS::add_policy(uint32_t p) {
    policy_vector.push_back(p);
}
