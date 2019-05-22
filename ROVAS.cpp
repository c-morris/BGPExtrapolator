#include "ROVAS.h"
#include 


bool ROVAS::pass_rov(Announcement &ann) {
  extern global_victim_asn;
  if (ann->origin == global_attacker_asn) {
    return false;
  } else {
    return true;
  }
}

/** Push the received announcements to the incoming_announcements vector.
 *
 * Note that this differs from the Python version in that it does not store
 * a dict of (prefix -> list of announcements for that prefix).
 *
 * @param announcements to be pushed onto the incoming_announcements vector.
 */
void ROVAS::receive_announcements(std::vector<Announcement> &announcements) {
    for (Announcement &ann : announcements) {
        // Check if the Announcement is valid
        if (pass_rov(ann)) {
          // Do not check for duplicates here
          // push_back makes a copy of the announcement
          incoming_announcements->push_back(ann);
        }
    }
}
