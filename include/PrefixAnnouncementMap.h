#ifndef PREFIX_ANNOUNCEMENT_MAP_H
#define PREFIX_ANNOUNCEMENT_MAP_H

#include <vector>

#include "Prefix.h"
#include "Announcements/Announcement.h"
#include "Announcements/EZAnnouncement.h"
#include "Announcements/ROVppAnnouncement.h"

template <class AnnouncementType>
class PrefixAnnouncementMap : public std::vector<AnnouncementType> {
public:
    PrefixAnnouncementMap(size_t capacity) : std::vector<AnnouncementType>() {
        std::vector<AnnouncementType>::reserve(capacity);

        for(size_t i = 0; i < capacity; i++)
            std::vector<AnnouncementType>::push_back(AnnouncementType());
    }

    virtual ~PrefixAnnouncementMap();

    virtual AnnouncementType& find(const Prefix<> &prefix);

    virtual void reset_all();
    virtual void reset_announcement(Prefix<> &prefix);
    virtual void reset_announcement(Announcement &announcement);

    virtual bool filled(const AnnouncementType &announcement);
    virtual bool filled(const Prefix<> &prefix);

    virtual size_t num_filled();
};
#endif