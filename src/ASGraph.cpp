#include "ASGraph.h"
#include "RovAnnouncement.h"

template <typename T>
void ASGraph<T>::print() {
    std::cout << announcement.origin << std::endl;
}

template class ASGraph<Announcement>;
template class ASGraph<RovAnnouncement>;