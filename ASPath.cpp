#include "ASPath.h"

ASPath::ASPath() {
    as_path = new std::vector<uint32_t>;
}

ASPath::ASPath(std::string path_as_string) : ASPath() {
    // Remove brackets from string
    path_as_string.erase(std::find(path_as_string.begin(), path_as_string.end(), '}'));
    path_as_string.erase(std::find(path_as_string.begin(), path_as_string.end(), '{'));

    // Fill as_path vector from parsing string
    std::string delimiter = ",";
    std::string::size_type pos = 0;
    std::string token;
    while ((pos = path_as_string.find(delimiter)) != std::string::npos) {
        token = path_as_string.substr(0,pos);
        try {
            as_path->push_back(std::stoul(token));
        } catch(...) {
            std::cerr << "Parse path error, token was: " << token << std::endl;
        }
        path_as_string.erase(0,pos + delimiter.length());
    }
    // Handle last ASN after loop
    try {
        as_path->push_back(std::stoul(path_as_string));
    } catch(...) {
        std::cerr << "Parse path error, token was: " << path_as_string << std::endl;
    }
}

void ASPath::seedAnnouncements(ASGraph* graph, Prefix<> prefix, int64_t timestamp, bool random) {
    // Handle empty as_path
    if (as_path->empty()) { 
        return;
    }
    
    uint32_t i = 0;
    uint32_t path_l = as_path->size();
    
    // Announcement at origin for checking along the path
    Announcement ann_to_check_for(as_path->at(path_l-1),
                                  prefix.addr,
                                  prefix.netmask,
                                  0,
                                  timestamp); 
    
    // Iterate through path starting at the origin
    for (auto it = as_path->rbegin(); it != as_path->rend(); ++it) {
        // Increments path length, including prepending
        i++;
        // If ASN not in graph, continue
        if (graph->ases->find(*it) == graph->ases->end()) {
            continue;
        }
        // Translate ASN to it's supernode
        uint32_t asn_on_path = graph->translate_asn(*it);
        // Find the current AS on the path
        AS *as_on_path = graph->ases->find(asn_on_path)->second;
        // Check if already received this prefix
        if (as_on_path->already_received(ann_to_check_for)) {
            // Find the already received announcement
            auto search = as_on_path->all_anns->find(ann_to_check_for.prefix);
            // If the current timestamp is newer (worse)
            if (ann_to_check_for.tstamp > search->second.tstamp) {
                // Skip it
                continue;
            } else if (ann_to_check_for.tstamp == search->second.tstamp) {
                // Tie breaker for equal timestamp
                bool value = true;
                // Random tiebreak if enabled

                if (random == true) {
                    value = as_on_path->get_random();
                }

                // First come, first saved if random is disabled
                if (value) {
                    continue;
                } else {
                    // Position of previous AS on path
                    uint32_t pos = path_l - i + 1;
                    // Prepending check, use original priority
                    if (pos < path_l && as_path->at(pos) == as_on_path->asn) {
                        continue;
                    }
                    as_on_path->delete_ann(ann_to_check_for);
                }
            } else {
                // Delete worse MRT announcement, proceed with seeding
                as_on_path->delete_ann(ann_to_check_for);
            }
        }
        
        // If ASes in the path aren't neighbors (data is out of sync)
        bool broken_path = false;

        // It is 3 by default. It stays as 3 if it's the origin.
        int received_from = 300;
        // If this is not the origin AS
        if (i > 1) {
            // Get the previous ASes relationship to current AS
            if (as_on_path->providers->find(*(it - 1)) != as_on_path->providers->end()) {
                received_from = AS_REL_PROVIDER;
            } else if (as_on_path->peers->find(*(it - 1)) != as_on_path->peers->end()) {
                received_from = AS_REL_PEER;
            } else if (as_on_path->customers->find(*(it - 1)) != as_on_path->customers->end()) {
                received_from = AS_REL_CUSTOMER;
            } else {
                broken_path = true;
            }
        }

        // This is how priority is calculated
        uint32_t path_len_weighted = 100 - (i - 1);
        uint32_t priority = received_from + path_len_weighted;
       
        uint32_t received_from_asn = 0;
        // If this AS is the origin
        if (it == as_path->rbegin()){
            // It received the ann from itself
            received_from_asn = *it;
        } else {
            // Otherwise received it from previous AS
            received_from_asn = *(it-1);
        }
        // No break in path so send the announcement
        if (!broken_path) {
            Announcement ann = Announcement(*as_path->rbegin(),
                                            prefix.addr,
                                            prefix.netmask,
                                            priority,
                                            received_from_asn,
                                            timestamp,
                                            true);
            // Send the announcement to the current AS
            as_on_path->process_announcement(ann, random);
            if (graph->inverse_results != NULL) {
                auto set = graph->inverse_results->find(
                        std::pair<Prefix<>,uint32_t>(ann.prefix, ann.origin));
                // Remove the AS from the prefix's inverse results
                if (set != graph->inverse_results->end()) {
                    set->second->erase(as_on_path->asn);
                }
            }
        } else {
            // Report the broken path
            //std::cerr << "Broken path for " << *(it - 1) << ", " << *it << std::endl;
            
            static int g_broken_path = 0;

            Logger::getInstance().log("Broken_Paths") << "Broken Path #" << g_broken_path << ", For: " << *(it - 1) << ", " << *it;

            g_broken_path++;
        }
    }
}

bool ASPath::containsLoop() {
    uint32_t prev = 0;
    bool loop = false;
    int sz = as_path->size();
    for (int i = 0; i < (sz-1) && !loop; i++) {
        prev = as_path->at(i);
        for (int j = i+1; j < sz && !loop; j++) {
            loop = as_path->at(i) == as_path->at(j) && as_path->at(j) != prev;
            prev = as_path->at(j);
        }
    }

    return loop;
}

std::vector<uint32_t>* ASPath::getASPath() { return as_path; }