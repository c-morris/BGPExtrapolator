#include "PrefixAnnouncementMap.h"

/******************** Iterator ********************/
template <class AnnouncementType, typename PrefixType>
typename PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator& PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator::operator++() {
    if (index == parent->announcements.size()) {
        return *this;
    }
    index++;
    while(index < parent->announcements.size()) {
        if(parent->announcements.at(index).tstamp != -1) {
            return *this;
        }

        index++;
    }

    return *this;
}

template <class AnnouncementType, typename PrefixType>
typename PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator::operator++(int) {
    Iterator tmp(*this); 
    operator++(); 
    return tmp;
}

template <class AnnouncementType, typename PrefixType>
bool PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator::operator==(const PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator& other) const { return index == other.index; }

template <class AnnouncementType, typename PrefixType>
bool PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator::operator!=(const PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator& other) const { return index != other.index; }

template <class AnnouncementType, typename PrefixType>
const AnnouncementType& PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator::operator*() { return parent->announcements.at(index); }

template <class AnnouncementType, typename PrefixType>
const AnnouncementType* PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator::operator->() { return &parent->announcements.at(index); }

/******************** PrefixAnnouncementMap ********************/
template <class AnnouncementType, typename PrefixType>
PrefixAnnouncementMap<AnnouncementType, PrefixType>::~PrefixAnnouncementMap() { }

template <class AnnouncementType, typename PrefixType>
typename PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator PrefixAnnouncementMap<AnnouncementType, PrefixType>::find(const Prefix<PrefixType> &prefix) {
    return Iterator(this, prefix.block_id);
}

template <class AnnouncementType, typename PrefixType>
void PrefixAnnouncementMap<AnnouncementType, PrefixType>::insert(const Prefix<PrefixType> &prefix, const AnnouncementType &ann) {
    if(prefix.block_id != ann.prefix.block_id) {
        BOOST_LOG_TRIVIAL(error) << "This announcement cannot be inserted into this iterator since the index in the prefix is different from the index of the prefix in the announcement!";
        exit(20);
    }

    // if(announcements.at(prefix.block_id).tstamp == -1)
    //     filled_size++;

    // if(ann.tstamp == -1)
    //     filled_size--;

    if(announcements.at(prefix.block_id).tstamp != -1 && ann.tstamp == -1) 
        filled_size--;
    else if(announcements.at(prefix.block_id).tstamp == -1 && ann.tstamp != -1)
        filled_size++;
    
    announcements.at(prefix.block_id) = ann;
}

template <class AnnouncementType, typename PrefixType>
void PrefixAnnouncementMap<AnnouncementType, PrefixType>::insert(const Iterator &other_iterator) {
    if(other_iterator.index >= announcements.size()) {
        BOOST_LOG_TRIVIAL(error) << "The element of the other iterator cannot be inserted into this map since its index is out of bounds of this map!";
        exit(21);
    }

    const AnnouncementType &other_announcement = other_iterator.parent->announcements.at(other_iterator.index);
    insert(other_announcement.prefix, other_announcement);
}

template <class AnnouncementType, typename PrefixType>
typename PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator PrefixAnnouncementMap<AnnouncementType, PrefixType>::begin() const {
    size_t index = 0;
    while (index < announcements.size() && announcements.at(index).tstamp == -1) {
            index++;
    }
    return Iterator(this, index);
}

template <class AnnouncementType, typename PrefixType>
typename PrefixAnnouncementMap<AnnouncementType, PrefixType>::Iterator PrefixAnnouncementMap<AnnouncementType, PrefixType>::end() const {
    return Iterator(this, announcements.size());
}

template <class AnnouncementType, typename PrefixType>
void PrefixAnnouncementMap<AnnouncementType, PrefixType>::clear() {
    for(auto iterator = announcements.begin(); iterator != announcements.end(); ++iterator)
        iterator->tstamp = -1;

    filled_size = 0;
}

template <class AnnouncementType, typename PrefixType>
void PrefixAnnouncementMap<AnnouncementType, PrefixType>::erase(Prefix<PrefixType> &prefix) {
    AnnouncementType& ann = announcements.at(prefix.block_id);

    if(ann.tstamp != -1) {
        filled_size--;
        ann.tstamp = -1;
    }
}

template <class AnnouncementType, typename PrefixType>
void PrefixAnnouncementMap<AnnouncementType, PrefixType>::erase(AnnouncementType &announcement) {
    erase(announcement.prefix);
}

template <class AnnouncementType, typename PrefixType>
bool PrefixAnnouncementMap<AnnouncementType, PrefixType>::filled(const AnnouncementType &announcement) {
    return !(announcement.tstamp == -1);
}

template <class AnnouncementType, typename PrefixType>
bool PrefixAnnouncementMap<AnnouncementType, PrefixType>::filled(const Prefix<PrefixType> &prefix) {
    return !(announcements.at(prefix.block_id).tstamp == -1);
}

template <class AnnouncementType, typename PrefixType>
size_t PrefixAnnouncementMap<AnnouncementType, PrefixType>::size() {
    return filled_size;
}

template <class AnnouncementType, typename PrefixType>
size_t PrefixAnnouncementMap<AnnouncementType, PrefixType>::capacity() {
    return announcements.capacity();
}

template <class AnnouncementType, typename PrefixType>
bool PrefixAnnouncementMap<AnnouncementType, PrefixType>::empty() {
    return filled_size == 0;
}

template class PrefixAnnouncementMap<Announcement<>>;
template class PrefixAnnouncementMap<Announcement<uint128_t>, uint128_t>;
template class PrefixAnnouncementMap<ROVAnnouncement>;
template class PrefixAnnouncementMap<ROVppAnnouncement>;
template class PrefixAnnouncementMap<EZAnnouncement>;
