#include "CommunityDetection.h"

//**************** Community Detection ****************//
CommunityDetection::CommunityDetection(EZExtrapolator *extrapolator, uint32_t local_threshold) : extrapolator(extrapolator), local_threshold(local_threshold) { }

bool CommunityDetection::contains_hyper_edge(std::vector<uint32_t> &hyper_edge) {
    for(auto &temp_edge : hyper_edges)
        if(std::equal(hyper_edge.begin(), hyper_edge.end(), temp_edge.begin()))
            return true;
    return false;
}

void CommunityDetection::add_hyper_edge(std::vector<uint32_t> &hyper_edge) {
    if(contains_hyper_edge(hyper_edge))
        return;

    hyper_edges.push_back(hyper_edge);
}

void CommunityDetection::add_report(EZAnnouncement &announcement, EZASGraph *graph) {
    std::vector<uint32_t> as_path = announcement.as_path;

    //Check if the AS at the end of the path is an adopter. 
    //A non-adopter cannot make a report
    uint32_t reporting_asn = as_path.at(0);
    auto reporting_as_search = graph->ases->find(reporting_asn);
    if(reporting_as_search == graph->ases->end())
        return;

    //The reporting AS must be an adopter in order to make a report...
    EZAS *reporting_as = reporting_as_search->second;
    if(reporting_as->policy == EZAS_TYPE_BGP)
        return;

    //******   Trim Report to Hyper Edges   ******//
    //End of the path is the origin
    //The condition of an invalid MAC is this: 
    //    When an adopter at index i is not a true neighbor of the AS at i - 1.
    //Consider an adopting origin, which is at the very end of the path

    //Indecies of the hyper edge range
    std::vector<std::pair<int, int>> hyper_edge_ranges;

    //Store the index of the adopter next to the invalid MAC so the range can be stored if/when an adopter is found on the other side
    int as_index_invalid_relationship = 0;

    unsigned int cur_pol;
    unsigned int prev_pol;

    //There is an invalid MAC, keep an eye out for an adopter on the other side of the invalid MAC
    bool search_for_adopter = false;

    for(int i = as_path.size() - 2; i >= 0; i--) {
        bool nbrs = true;
        auto current_search = graph->ases->find(as_path.at(i));
        if(current_search == graph->ases->end()) {
            cur_pol = EZAS_TYPE_BGP;
            nbrs = false;
        } else {
            EZAS *current_as = current_search->second;
            cur_pol = current_as->policy;
        }

        auto previous_search = graph->ases->find(as_path.at(i + 1));
        if(previous_search == graph->ases->end()) {
            prev_pol = EZAS_TYPE_BGP;
            nbrs = false;
        } else {
            EZAS *previous_as = previous_search->second;
            prev_pol = previous_as->policy;
            if (nbrs)
                nbrs = previous_as->has_neighbor(current_search->second->asn);
        }


        //Previous AS is an adopter and does not have an actual relationship to this AS (invalid MAC)
        if(prev_pol != EZAS_TYPE_BGP && !nbrs) {
            as_index_invalid_relationship = i + 1;
            search_for_adopter = true;
            continue;
        }

        if(cur_pol != EZAS_TYPE_BGP && search_for_adopter) {
            search_for_adopter = false;
            hyper_edge_ranges.push_back(std::make_pair(i, as_index_invalid_relationship));
        }
    }

    // These are not the MACs you are looking for....
    // In other words, while there are invalid MACs, they cannot be seen and are not between adopters. 
    if(hyper_edge_ranges.size() == 0)
        return; 

    for(std::pair<int, int> &range : hyper_edge_ranges) {
        std::vector<uint32_t> hyper_edge(as_path.begin() + range.first, as_path.begin() + range.second + 1);

        //check if we have this edge already
        bool contains = false;
        for(auto &temp_edge : edges_to_process) {
            if(std::equal(hyper_edge.begin(), hyper_edge.end(), temp_edge.begin())) {
                contains = true;
                break;
            }
        }

        if(contains)
            continue;

        edges_to_process.push_back(hyper_edge);
    }
}

void CommunityDetection::process_reports(EZASGraph *graph) {
    for(auto &edge : edges_to_process) {
        blacklist_paths.insert(edge);

        unsigned int reporter_policy = graph->ases->find(edge.at(0))->second->policy;

        if(reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_LOCAL || reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL || reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL_LOCAL) {
            //std::vector<uint32_t> truncated = std::vector<uint32_t>(edge.begin()+1, edge.end());
            add_hyper_edge(edge);
        }
    }

    if(hyper_edges.size() > 0)
        local_threshold_approx_filtering();
    
    edges_to_process.clear();
}

void CommunityDetection::local_threshold_approx_filtering() {

    // 1. Sort hyperedges from longest to shortest
    // 2. Collapse edges from longest to shortest
    // 3. Add results to blacklists

    // The AS graph is shallow, we can use bucket sort for linear time sorting.
    // Path length is capped at 64. 
    // Justification: a path length exceeding the default TTL of many machines is of little practical use
    const int num_buckets = 64;
    std::vector<std::set<std::vector<uint32_t>>> buckets(num_buckets); 

    for(const auto &edge : this->hyper_edges) {
        if (edge.size() <= 64) {
            buckets[edge.size()].insert(edge);
        }
    }

    /* Collapse Hyperedges (locally)
       NOTE: DOES NOT WORK FOR MULTIPLE ATTACKERS (yet)
       
       Ex: AS 666 forwards an attack to three of its neighbors (x, y, z) where thresh=2
      
           x 
            \
           y-666-...
            /  
           z
      
       This will be detected here, and the edge will be collapsed to one size smaller
       (e. g., 666 and the rest of the path). 
    */
    for (auto it = buckets.rbegin(); it != buckets.rend(); ++it) {
        std::map<uint32_t, std::set<uint32_t>> counts;
        for (const auto &edge : *it) {
            if (edge.size() >= 2) {
                counts[edge[1]].insert(edge[0]);
            }
        }
        for (const auto &suspect : counts) {
            if (suspect.second.size() > local_threshold) {
                // Add all the edges (to make sure none are missed)
                // This is one area that needs to be re-done for muiltiple attackers
                 for (const auto &edge : *it) {
                    if (edge.size() >= 2) {
                        if (edge[1] == suspect.first) {
                            std::vector<uint32_t> tmp(next(edge.begin()), edge.end());
                            (it+1)->insert(tmp);
                        }
                    }
                }
            }
        }
    }
    /* Now check the other direction (except for the origin)
       
       Ex: AS 666 fakes attacks from three customers (a, b, c) where thresh=2
      
                 a 
                /
         ...-666-b
                \
                 c
      
       This will be detected here, and the edge will be collapsed to as small as possible
       (e. g., the rest of the path followed by 666). 
    */

    for (int i = 2; i < num_buckets; i++) {
        std::map<uint32_t, std::set<uint32_t>> counts;
        for (const auto &edge : buckets[i]) {
            for (int j = 0; j < i-2; j++) {
                counts[edge[j]].insert(edge[j+1]);
                if (edge[j] == 666) {
                    //std::cout << "got one i = " << i << " and j = " << j << "and the edge was " << edge[j+1] << "\n";
                }
            }
        }
        for (const auto &suspect : counts) {
            if (suspect.second.size() > local_threshold) {
                // Add all the edges (to make sure none are missed)
                // This is one area that needs to be re-done for muiltiple attackers
                 for (const auto &edge : buckets[i]) {
                    for (int j = 0; j < i-2; j++) {
                        if (edge[j] == suspect.first) {
                            std::vector<uint32_t> tmp(edge.begin(), edge.begin()+j+1);
                            buckets[tmp.size()].insert(tmp);
                            if (tmp.size() > 0) {
                            //    std::cout << "adding " << tmp[0] << " to bucket " << tmp.size() << "\n";
                            }
                        }
                    }
                }
            }
        }
    }

    // Blacklist all edges that were fully collapsed
    for (const auto &edge : buckets[1]) {
        for (const auto asn : edge) {
            blacklist_asns.insert(asn);
        }
    }
    // Blacklist all the paths that were partially collapsed
    for (auto it = buckets.begin()+2; it != buckets.end(); ++it) {
        for (const auto &edge : *it) {
            blacklist_paths.insert(edge);
        }
    }
}
