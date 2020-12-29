#ifndef PREFIX_ANNOUNCEMENT_MAP_H
#define PREFIX_ANNOUNCEMENT_MAP_H

#include <vector>

#include "Prefix.h"
#include "Announcements/Announcement.h"
#include "Announcements/EZAnnouncement.h"
#include "Announcements/ROVppAnnouncement.h"

template <class AnnouncementType>
class PrefixAnnouncementMap {
private:
    size_t filled_size;//Number of announcements that are filled in the announcements vector
    std::vector<AnnouncementType> announcements;

public:
    class Iterator {
    public:
        const PrefixAnnouncementMap<AnnouncementType> *parent;
        size_t index;

        Iterator(const PrefixAnnouncementMap<AnnouncementType> *parent, size_t index) : parent(parent), index(index) {
            if(index < parent->announcements.size() && parent->announcements.at(index).tstamp == -1)
                this->index = parent->announcements.size();
        }

        Iterator(const Iterator& other) : parent(other.parent), index(other.index) {

        }

        virtual Iterator& operator++();
        virtual Iterator operator++(int);

        virtual bool operator==(const Iterator& other) const;
        virtual bool operator!=(const Iterator& other) const;

        virtual const AnnouncementType& operator*();
        virtual const AnnouncementType* operator->();
    };

    /**
     * Given the capacity, this will initilize a list to allocate that many announcements of the given type.
     * This will initilize with the default constructor of the announcement type.
     * However, the return value of size after this constructor will be 0.
     * 
     * This is because the size function counts the amount of "initilized" announcements.
     * After this constructor, the announcements stored here are meaningless blocks of allocated memory.
     * However, the point is that these blocks of memory are allocated and freed only once. Thus, during execution,
     * memory does not need to be played with as much. In addition, this allows us to access announcements by index
     * since they will be populated in a vector, and their corresponding index is stored in the prefix obejct.
     */
    PrefixAnnouncementMap(size_t capacity) : filled_size(0) {
        announcements.reserve(capacity);

        for(size_t i = 0; i < capacity; i++)
            announcements.push_back(AnnouncementType());
    }

    virtual ~PrefixAnnouncementMap();

    /**
     * Returns an iterator to the element at the given prefix. If the announcement at this prefix is "not initilized"
     *  then this will return the end iterator.
     */
    virtual Iterator find(const Prefix<> &prefix);

    virtual void insert(const Prefix<> &prefix, const AnnouncementType &ann);
    virtual void insert(const Iterator &other_iterator);

    virtual Iterator begin() const;
    virtual Iterator end() const;

    /**
     * Resets all stored announcements into a fake "uninitialized" state. The memory is still allocated,
     *  the announcements are all flagged as being uninitialized. Thus, the size will go to 0. The idea here
     *  is that the announcements will no longer be used or seen, but may be overwritten later with a valid one. 
     *  The point is, the memory is only allocated on startup and the absolute end of extrapolation.
     */
    virtual void clear();

    /**
     *  Resets the announcement at this prefix into an "uninitialized" state.
     * 
     *  As of writing this, this means setting the timestamp to -1, which cannot happen
     */
    virtual void erase(Prefix<> &prefix);

    /**
     *  Resets the announcement at the prefix in the announcement given announcement, into an "uninitialized" state.
     * 
     *  The given announcement is not modified, unless it happens to be a refrence to the announcement that is in this structure.
     * 
     *  As of writing this, this means setting the timestamp to -1, which cannot happen
     */
    virtual void erase(Announcement &announcement);

    /**
     * Determines whether or not the given announcement is a placeholder announcement or is a populated announcement
     * The idea is that we make the upfront allocation of announcments into memory, then modify the state of each announcement
     * This function will tell you if the given announcement is just a placeholder or is a real announcement with meaningful data
     */
    virtual bool filled(const AnnouncementType &announcement);

    /**
     * Determines whether or not the announcement, at the given prefix, is a placeholder announcement or is a populated announcement
     * The idea is that we make the upfront allocation of announcments into memory, then modify the state of each announcement
     * This function will tell you if the announcement, at the given prefix, is just a placeholder or is a real announcement with meaningful data
     */
    virtual bool filled(const Prefix<> &prefix);

    /**
     *  This will give back the number of announcements that are populated in the data structure.
     * 
     *  One would expect that the size of this structure to be constant throughout execution, and this is true.
     * 
     *  However, the idea of this function is to hide this fact. This will count the number of valid announcements.
     * 
     *  BE WARNED, the valid announcements in this structure are likely sparse, so be sure to check if the announcement
     *      at a given index is filled
     */
    virtual size_t size();

    /**
     *  This returns the capacity of the internal vector.
     *  THIS SHOULD NEVER BE DIFFERENT FROM THE PASSED IN VALUE FROM THE CONSTRUCTOR
     *  IF it is different than what was originally passed in, then there is a bug
     */
    virtual size_t capacity();

    /**
     *  Returns whether or not there are any filled announcements inside this data structure
     *  True if there are no filled announcements, false if there is at least one announcement
     */
    virtual bool empty();
};
#endif