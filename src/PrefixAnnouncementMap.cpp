#include "PrefixAnnouncementMap.h"

template <class AnnouncementType>
PrefixAnnouncementMap<AnnouncementType>::~PrefixAnnouncementMap() { }

template <class AnnouncementType>
AnnouncementType& PrefixAnnouncementMap<AnnouncementType>::find(const Prefix<> &prefix) {
    return std::vector<AnnouncementType>::at(prefix.block_id);
}

template <class AnnouncementType>
void PrefixAnnouncementMap<AnnouncementType>::reset_all() {
    for(auto iterator = std::vector<AnnouncementType>::begin(); iterator != std::vector<AnnouncementType>::end(); ++iterator)
        iterator->tstamp = -1;
}

template <class AnnouncementType>
void PrefixAnnouncementMap<AnnouncementType>::reset_announcement(Prefix<> &prefix) {
    find(prefix).tstamp = -1;
}

template <class AnnouncementType>
void PrefixAnnouncementMap<AnnouncementType>::reset_announcement(Announcement &announcement) {
    reset_announcement(announcement.prefix);
}

template <class AnnouncementType>
bool PrefixAnnouncementMap<AnnouncementType>::filled(const AnnouncementType &announcement) {
    return !(announcement.tstamp == -1);
}

template <class AnnouncementType>
bool PrefixAnnouncementMap<AnnouncementType>::filled(const Prefix<> &prefix) {
    return filled(find(prefix));
}

template <class AnnouncementType>
size_t PrefixAnnouncementMap<AnnouncementType>::num_filled() {
    size_t num_filled = 0;
    for(auto iterator = std::vector<AnnouncementType>::begin(); iterator != std::vector<AnnouncementType>::end(); ++iterator)
        if(filled(*iterator)) num_filled++;

    return num_filled;
}

template class PrefixAnnouncementMap<Announcement>;
template class PrefixAnnouncementMap<ROVppAnnouncement>;
template class PrefixAnnouncementMap<EZAnnouncement>;