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

#include "Prefix.h"

/** Unit tests for Prefix.h
 */

/** Tests Prefix struct constructor.
 *
 * @return true if successful, otherwise false.
 */
bool test_prefix(){
    // Check correctness
    Prefix<> prefix = Prefix<>("1.1.1.0", "255.255.255.0");
    if (prefix.addr != 0x01010100 || prefix.netmask != 0xffffff00)
        return false;

    // Check out of 8-bit range 
    Prefix<> p1 = Prefix<>("256.1.1.0", "255.255.255.0");
    if (p1.addr != 0x0 || p1.netmask != 0xffffff00)
        return false;
    
    // Check Malformed 
    Prefix<> p2 = Prefix<>("1.1.1.1.1", "255.255.255.0");
    if (p2.addr != 0x0 || p2.netmask != 0xffffff00)
        return false;
    
    // Check Malformed 
    Prefix<> p3 = Prefix<>("1. .1.1", "255.255.255.0");
    if (p3.addr != 0x0 || p3.netmask != 0xffffff00)
        return false;

    // Check Malformed 
    Prefix<> p4 = Prefix<>("1.1.1. ", "255.255.255.0");
    if (p4.addr != 0x0 || p4.netmask != 0xffffff00)
        return false;

    // Check Empty
    Prefix<> p5 = Prefix<>("", "255.255.255.0");
    if (p5.addr != 0x0 || p5.netmask != 0xffffff00)
        return false;

   return true;
}


/** Tests the conversion of an address as a string to a cidr.
 *
 * @return true if successful, otherwise false.
 */
bool test_string_to_cidr(){
    Prefix<> prefix = Prefix<>("1.1.1.0", "255.255.255.0");
    if (prefix.to_cidr() != "1.1.1.0/24")
        return false;
    return true;
}


/** Tests the definition of the < operator.
 *
 * @return true if successful, otherwise false.
 */
bool test_prefix_lt_operator(){
    Prefix<> a = Prefix<>("1.1.1.0", "255.255.255.0");
    Prefix<> b = Prefix<>("1.1.1.0", "255.255.254.0");
    if (a < b)
        return false;
    return true;
}


/** Tests the definition of the > operator.
 *
 * @return true if successful, otherwise false.
 */
bool test_prefix_gt_operator(){
    Prefix<> a = Prefix<>("1.1.1.0", "255.255.255.0");
    Prefix<> b = Prefix<>("1.1.1.0", "255.255.254.0");
    if (b > a)
        return false;
    return true;
}


/** Tests the definition of the == operator.
 *
 * @return true if successful, otherwise false.
 */
bool test_prefix_eq_operator(){
    Prefix<> a = Prefix<>("1.1.1.0", "255.255.255.0");
    Prefix<> b = Prefix<>("1.1.1.0", "255.255.254.0");
    if (a == b)
        return false;
    return true;
}

/** Tests the contained_in_or_equal_to function.
 *
 * @return true if successful, otherwise false.
 */
bool test_prefix_contained_in_or_equal_to_operator(){
    Prefix<> a = Prefix<>("1.1.1.0", "255.255.255.0");
    Prefix<> b = Prefix<>("1.1.2.0", "255.255.254.0");
    Prefix<> c = Prefix<>("1.1.0.0", "255.255.0.0");
    if (!a.contained_in_or_equal_to(c))
        return false;
    if (c.contained_in_or_equal_to(a))
        return false;
    if (!a.contained_in_or_equal_to(a))
        return false;
    if (b.contained_in_or_equal_to(a))
        return false;
    if (a.contained_in_or_equal_to(b))
        return false;
    return true;
}