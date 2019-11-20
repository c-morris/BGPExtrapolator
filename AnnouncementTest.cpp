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
