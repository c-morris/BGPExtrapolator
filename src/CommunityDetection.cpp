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
        EZAS *current_as;
        if(current_search == graph->ases->end()) {
            cur_pol = EZAS_TYPE_BGP;
            nbrs = false;
        } else {
            current_as = current_search->second;
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
            //search_for_adopter = false;
            hyper_edge_ranges.push_back(std::make_pair(i, as_index_invalid_relationship));
            // Also add this edge to the Adopter's own edges (excluding its own ASN)
            std::vector<uint32_t> hyper_edge_at_current_as(as_path.begin() + i, as_path.begin() + as_index_invalid_relationship);
            current_as->edges_to_process.push_back(hyper_edge_at_current_as);
        }
    }

    // These are not the MACs you are looking for....
    // In other words, while there are invalid MACs, they cannot be seen and are not between adopters. 
    if(hyper_edge_ranges.size() == 0)
        return; 

    for(std::pair<int, int> &range : hyper_edge_ranges) {
        std::vector<uint32_t> hyper_edge(as_path.begin() + range.first, as_path.begin() + range.second + 1);

        //check if we have this edge already in the directory
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

    // Now process local edges at each AS
    for (auto const& as : *graph->ases) {
        for (auto &edge : as.second->edges_to_process) {
            // Must be sorted for std::includes
            // (this is still correctly testing for a subset of the path)
            std::sort(edge.begin(), edge.end());
            as.second->blacklist_paths.insert(std::move(edge));
        }
        as.second->edges_to_process.clear();
    }

    if(hyper_edges.size() > 0)
        local_threshold_approx_filtering();
    
    edges_to_process.clear();
}

bool CommunityDetection::is_cover(std::set<uint32_t> sprime, std::set<uint32_t> s) {
    // does s prime form a set cover over s
    std::set<uint32_t> covered;
    for (auto prime_candidate : sprime) {
        for (const auto &edge : this->hyper_edges) {
            // if this prime candidate is in the edge, add these ASNs to covered set
            if (std::find(edge.begin(), edge.end(), prime_candidate) != edge.end()) {
                for (auto edge_asn : edge) {
                    covered.insert(edge_asn);
                }
                continue;
            }
        }
    }
    // QUESTION: if s is supposed to be the set of all ASNs in this component, this works
    // if s is the set of all ASNs excluding those of s prime, this does not work
    return std::includes(covered.begin(), covered.end(), s.begin(), s.end());
}

// return a map of ASN to set of those it is indistinguishable from
std::map<uint32_t, std::set<uint32_t>> CommunityDetection::gen_ind_asn(std::set<uint32_t> s, const std::vector<std::vector<uint32_t>> &edges) {
    std::map<uint32_t, std::set<uint32_t>> retval;
    for (uint32_t asn : s) {
        auto s_copy = s;
        for (auto edge : edges) {
            if (std::find(edge.begin(), edge.end(), asn) != edge.end()) {
                // if asn in edge, only save other ASNs also in the edge
                for (auto it = s_copy.begin(); it != s_copy.end() ;) {
                    if (*it != asn && std::find(edge.begin(), edge.end(), *it) == edge.end()) {
                        // if asn missing from edge but present in s_copy, remove from s_copy
                        it = s_copy.erase(it);
                    } else { 
                        ++it;
                    }
                }
            }
        }
        retval.insert(std::pair<uint32_t, std::set<uint32_t>>(asn, s_copy));
    }
    return retval;
}

// generate a set of unique ASNs in the set of edges
std::set<uint32_t> CommunityDetection::get_unique_asns(std::vector<std::vector<uint32_t>> edges) {
    std::set<uint32_t> unique_asns;
    for (auto edge : edges) {
        for (auto asn : edge) {
            unique_asns.insert(asn);
        }
    }
    return unique_asns;
}

// return a map of ASN to its degree in the specified set of edges
std::map<uint32_t, uint32_t> CommunityDetection::get_degrees(std::set<uint32_t> s, const std::vector<std::vector<uint32_t>> &edges) {
    std::map<uint32_t, uint32_t> retval;
    for (uint32_t asn : s) {
        retval.insert(std::pair<uint32_t,uint32_t>(asn, 0));
    }
    for (auto edge : edges) {
        for (auto asn : edge) {
            retval[asn]++;
        }
    }
    return retval;
}

// generate all possible cover candidates of size sz with given indistinguishability map
std::vector<std::set<uint32_t>> CommunityDetection::gen_cover_candidates(size_t sz, 
        std::map<uint32_t, std::set<uint32_t>> &ind_map,
        std::map<uint32_t, uint32_t> &degrees) {

    std::vector<std::set<uint32_t>> retval;
    std::vector<std::pair<uint32_t,uint32_t>> sorted_degrees(degrees.begin(), degrees.end());
    std::sort(sorted_degrees.begin(), sorted_degrees.end(), [](auto a, auto b) {
        // Sort from greatest degree to least
        return a.second > b.second;
    });

    auto distinguishable_subsets = sorted_degrees;
    // Indistinguishable nodes must be grouped together, so remove 'duplicates'
    for (auto it = distinguishable_subsets.begin(); it != distinguishable_subsets.end(); ++it) {
        for (auto it2 = it+1; it2 != distinguishable_subsets.end();) {
            if (std::find(ind_map[it->first].begin(), ind_map[it->first].end(), it2->first) != ind_map[it->first].end()) {
                it2 = distinguishable_subsets.erase(it2);
            } else {
                ++it2;
            }
        }
    }

    // This greedy approach only works with one attacker
    // Start with the node with the highest degree plus all nodes it is indistinguishable from
    std::set<uint32_t> tmp;
    unsigned count = 0;
    for (auto d : distinguishable_subsets) {
        for (auto asn : ind_map[d.first]) {
            if (degrees[asn] > local_threshold) {
                tmp.insert(asn);
            } else {
                break;
            }
        }
        retval.push_back(tmp);
        if (++count > sz) { break; }
    }

    return retval;
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
                            // In this case, when going backwards, we must always add the origin
                            // This is a hacky heuristic optimization that works only for one origin being attacked
                            tmp.push_back(*edge.rbegin());
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
