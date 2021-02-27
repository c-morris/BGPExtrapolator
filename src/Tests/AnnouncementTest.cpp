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

#include "Announcements/Announcement.h"

/** Unit tests for Announcements.h
 */


/** Test the constructor for the Announcement struct
 *
 * @ return True for success
 */
bool test_announcement(){
    Announcement<> ann = Announcement<>(111, 0x01010101, 0xffffff00, 262, 222, false);
    if (ann.origin != 111 || ann.prefix.addr != 0x01010101 || ann.prefix.netmask != 0xffffff00 || ann.received_from_asn != 222 || ann.priority != 262 || ann.from_monitor != false)
        return false;
    return true;
}


/** Tests the << operator for the Announcements
 *
 * For use in debugging, this operator allows the printing of an announcements to an output stream
 *
 * @ return True for success 
 */
bool test_ann_os_operator(){
    return true;
}


/** Tests the csv row data for this announcement struct
 *
 * @ return True for success 
 */
bool test_to_csv(){
    return true;
}
