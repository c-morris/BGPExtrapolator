#include "PrefixAnnouncementMap.h"

/******************** Iterator ********************/
template <class AnnouncementType>
typename PrefixAnnouncementMap<AnnouncementType>::Iterator& PrefixAnnouncementMap<AnnouncementType>::Iterator::operator++() {
    index++;
    while(index < parent->announcements.size()) {
        if(parent->announcements.at(index).tstamp != -1) {
            return *this;
        }

        index++;
    }

    return *this;
}

template <class AnnouncementType>
typename PrefixAnnouncementMap<AnnouncementType>::Iterator PrefixAnnouncementMap<AnnouncementType>::Iterator::operator++(int) {
    Iterator tmp(*this); 
    operator++(); 
    return tmp;
}

template <class AnnouncementType>
bool PrefixAnnouncementMap<AnnouncementType>::Iterator::operator==(const PrefixAnnouncementMap<AnnouncementType>::Iterator& other) const { return index == other.index; }

template <class AnnouncementType>
bool PrefixAnnouncementMap<AnnouncementType>::Iterator::operator!=(const PrefixAnnouncementMap<AnnouncementType>::Iterator& other) const { return index != other.index; }

template <class AnnouncementType>
const AnnouncementType& PrefixAnnouncementMap<AnnouncementType>::Iterator::operator*() { return parent->announcements.at(index); }

template <class AnnouncementType>
const AnnouncementType* PrefixAnnouncementMap<AnnouncementType>::Iterator::operator->() { return &parent->announcements.at(index); }

/******************** PrefixAnnouncementMap ********************/
template <class AnnouncementType>
PrefixAnnouncementMap<AnnouncementType>::~PrefixAnnouncementMap() { }

template <class AnnouncementType>
typename PrefixAnnouncementMap<AnnouncementType>::Iterator PrefixAnnouncementMap<AnnouncementType>::find(const Prefix<> &prefix) {
    return Iterator(this, prefix.block_id);
}

template <class AnnouncementType>
void PrefixAnnouncementMap<AnnouncementType>::insert(const Prefix<> &prefix, const AnnouncementType &ann) {
    if(prefix.block_id != ann.prefix.block_id) {
        std::cerr << "This announcement cannot be inserted into this iterator since the index in the prefix is different from the index of the prefix in the announcement!" << std::endl;
        return;
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

template <class AnnouncementType>
void PrefixAnnouncementMap<AnnouncementType>::insert(const Iterator &other_iterator) {
    if(other_iterator.index >= announcements.size()) {
        std::cerr << "The element of the other iterator cannot be inserted into this map since its index is out of bounds of this map!" << std::endl;
        return;
    }

    const AnnouncementType &other_announcement = other_iterator.parent->announcements.at(other_iterator.index);
    insert(other_announcement.prefix, other_announcement);
}

template <class AnnouncementType>
typename PrefixAnnouncementMap<AnnouncementType>::Iterator PrefixAnnouncementMap<AnnouncementType>::begin() const {
    return Iterator(this, 0);
}

template <class AnnouncementType>
typename PrefixAnnouncementMap<AnnouncementType>::Iterator PrefixAnnouncementMap<AnnouncementType>::end() const {
    return Iterator(this, announcements.size());
}

template <class AnnouncementType>
void PrefixAnnouncementMap<AnnouncementType>::clear() {
    for(auto iterator = announcements.begin(); iterator != announcements.end(); ++iterator)
        iterator->tstamp = -1;

    filled_size = 0;
}

template <class AnnouncementType>
void PrefixAnnouncementMap<AnnouncementType>::erase(Prefix<> &prefix) {
    AnnouncementType& ann = announcements.at(prefix.block_id);

    if(ann.tstamp != -1) {
        filled_size--;
        ann.tstamp = -1;
    }
}

template <class AnnouncementType>
void PrefixAnnouncementMap<AnnouncementType>::erase(Announcement &announcement) {
    erase(announcement.prefix);
}

template <class AnnouncementType>
bool PrefixAnnouncementMap<AnnouncementType>::filled(const AnnouncementType &announcement) {
    return !(announcement.tstamp == -1);
}

template <class AnnouncementType>
bool PrefixAnnouncementMap<AnnouncementType>::filled(const Prefix<> &prefix) {
    return !(announcements.at(prefix.block_id).tstamp == -1);
}

template <class AnnouncementType>
size_t PrefixAnnouncementMap<AnnouncementType>::size() {
    return filled_size;
}

template <class AnnouncementType>
size_t PrefixAnnouncementMap<AnnouncementType>::capacity() {
    return announcements.capacity();
}

template <class AnnouncementType>
bool PrefixAnnouncementMap<AnnouncementType>::empty() {
    return filled_size == 0;
}

template class PrefixAnnouncementMap<Announcement>;
template class PrefixAnnouncementMap<ROVppAnnouncement>;
template class PrefixAnnouncementMap<EZAnnouncement>;