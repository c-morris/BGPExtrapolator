#include "CommunityDetection.h"

CommunityDetection::CommunityDetection() {
    threshold = 2;
    naive_threshhold_detection = new std::unordered_map<uint32_t, uint32_t>();
}

CommunityDetection::~CommunityDetection() {
    delete naive_threshhold_detection;
}

void CommunityDetection::add_report(EZAnnouncement &announcement) {
    for(uint32_t asn : announcement.as_path) {
        auto naive_search = naive_threshhold_detection->find(asn);
        if(naive_search == naive_threshhold_detection->end()) {
            naive_threshhold_detection->insert(std::pair<uint32_t, uint32_t>(asn, 1));
        } else {
            naive_search->second++;
        }
    }
}

void CommunityDetection::do_real_disconnections(EZASGraph* graph) {
    //Naive kill anyone with higher than threshold
    for(auto naive_pair : *naive_threshhold_detection) {
        if(naive_pair.second >= threshold) {
            graph->disconnect_as_from_adopting_neighbors(naive_pair.first);
        }
    }
}