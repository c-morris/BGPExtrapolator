#include "Prefix.h"

/** Unit tests for Prefix.h
 */

/** Tests Prefix struct constructor.
 *
 * @return true if successful, otherwise false.
 */
bool test_Prefix(){
    Prefix<> prefix = Prefix<>("1.1.1.0", "255.255.255.0");
    if (prefix.addr != 0x01010100 || prefix.netmask != 0xffffff00)
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
