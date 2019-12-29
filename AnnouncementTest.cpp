#include "Announcement.h"

/** Unit tests for Announcements.h
 */


/** Test the constructor for the Announcement struct
 *
 * @ return True for success
 */
bool test_announcement(){
    Announcement ann = Announcement(111, 0x01010101, 0xffffff00, 262, 222, false);
    if (ann.origin != 111 || ann.prefix.addr != 0x01010101 || ann.prefix.netmask != 0xffffff00 || ann.received_from_asn != 222 || ann.priority != 262 || ann.from_monitor != false)
        return false;
    return true;
}

/** Tests the == operator for the Announcements
 *
 * @ return True for success 
 */
bool test_ann_eq_operator(){
    Announcement a = Announcement(111, 0x01010101, 0xffffff00, 262, 222, false);
    Announcement b = Announcement(112, 0x01010101, 0xffffff00, 262, 222, false);
    Announcement c = Announcement(111, 0x01010102, 0xffffff00, 262, 222, false);
    Announcement d = Announcement(111, 0x01010101, 0xfffffff0, 262, 222, false);
    Announcement e = Announcement(111, 0x01010101, 0xffffff00, 261, 222, false);
    Announcement f = Announcement(111, 0x01010101, 0xffffff00, 262, 221, false);
    return !((!(a == a)) ||
        (a == b) ||
        (a == c) ||
        (a == d) ||
        (a == e) ||
        (a == f));
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
