#include "Announcement.h"

/** Unit tests for Announcements.h
 */


/** Test the constructor for the Announcement struct
 *
 * @ return True for success
 */
bool test_announcement(){
    return true;
}


/** Tests the < operator for Announcements struct
 *
 * @ return True for success
 */ 
bool test_ann_lt_operator(){
    // a is more specific
    Announcement a = Announcement(111, 0x01010101, 0xffffff00, 3, 222, true);
    Announcement b = Announcement(111, 0x01010100, 0xffffff00, 3, 222, true);
    // c is higher priority
    Announcement c = Announcement(111, 0x01010101, 0xffffff00, 3, 222, true);
    Announcement d = Announcement(111, 0x01010101, 0xffffff00, 2.5, 222, true);
    if (a < b)
        return false;
    if (c < d)
        return false;

    return true;
}


/** Tests the > operator for Announcements struct
 *
 * @ return True for success 
 */
bool test_ann_gt_operator(){
    return true;
}

/** Tests the == operator for Announcements struct
 *
 * @ return True for success 
 */
bool test_ann_eq_operator(){
    return true;
}


/** Tests the conversion to a string representation of this Announcement struct for SQL queries
 *
 * @ return True for success 
 */
bool test_to_sql(){
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
