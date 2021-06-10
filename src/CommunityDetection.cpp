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
    for(uint32_t asn : hyper_edge) {
        auto asn_search = asn_to_degree.find(asn);
        if(asn_search == asn_to_degree.end()) {
            std::shared_ptr<uint32_t> degree = std::make_shared<uint32_t>(1);
            asn_to_degree.insert(std::make_pair(asn, degree));
            sorted_asn_degree.push_back(std::make_pair(asn, degree));

            asn_to_cardinality.insert(std::make_pair(asn, hyper_edge.size()));
        } else {
            (*(asn_search->second))++;

            asn_to_cardinality.find(asn)->second += hyper_edge.size();
        }
    }
}

bool CommunityDetection::is_cover(std::unordered_set<uint32_t> &cover_candidate) {
    // We are not testing if s prime makes a cover, we are testing for what the MVC is by testing if any subset of S/S' with length threshold make a cover

    // // does s prime form a set cover over s
    // std::set<uint32_t> covered;
    // for (auto prime_candidate : sprime) {
    //     for (const auto &edge : this->hyper_edges) {
    //         // if this prime candidate is in the edge, add these ASNs to covered set
    //         if (std::find(edge.begin(), edge.end(), prime_candidate) != edge.end()) {
    //             for (auto edge_asn : edge) {
    //                 covered.insert(edge_asn);
    //             }
    //             continue;
    //         }
    //     }
    // }
    // // QUESTION: if s is supposed to be the set of all ASNs in this component, this works
    // // if s is the set of all ASNs excluding those of s prime, this does not work
    // return std::includes(covered.begin(), covered.end(), s.begin(), s.end());

    //Check if each edge in the component contains an element of the candidate. If not, then the candidate does not make a cover
    for(auto &edge : hyper_edges) {
        bool contains_any = false;
        for(uint32_t node : cover_candidate) {
            //This could be better if this was a set rather than a vector 
            if(std::find(edge.begin(), edge.end(), node) != edge.end()) {
                contains_any = true;
                break;
            }
        }

        //This edge does not contain any of the elements in the suspect list, thus this edge is not covered. 
        if(!contains_any)
            return false;
    }
    return true;
}

void CommunityDetection::generate_covers_helper(std::unordered_map<uint32_t, std::shared_ptr<uint32_t>>::iterator asn_to_degree_it,
                                                std::unordered_set<uint32_t> &building_subset, uint32_t subset_length, std::vector<std::unordered_set<uint32_t>> &covers) {

    //The subset we are building has reached the neccessary length
    if(building_subset.size() == subset_length) {
        if(is_cover(building_subset))
            covers.push_back(building_subset);
        return;
    }

    //Reached the end of the nodes to choose from
    if(asn_to_degree_it == asn_to_degree.end())
        return;

    uint32_t asn = asn_to_degree_it->first;
    asn_to_degree_it++;

    //Make a choice: include or exclude this asn
    building_subset.insert(asn);
    generate_covers_helper(asn_to_degree_it, building_subset, subset_length, covers);

    building_subset.erase(asn);
    return generate_covers_helper(asn_to_degree_it, building_subset, subset_length, covers);
}

std::vector<std::unordered_set<uint32_t>> CommunityDetection::generate_covers() {
    std::vector<std::unordered_set<uint32_t>> covers;
    std::unordered_set<uint32_t> building_subset;

    generate_covers_helper(asn_to_degree.begin(), building_subset, local_threshold, covers);

    return covers;
}

bool CommunityDetection::are_indistinguishable(uint32_t asn1, uint32_t asn2) {
    for(auto &edge : hyper_edges) {
        bool contain_asn1 = false;
        bool contain_asn2 = false;

        for(auto asn : edge) {
            if(asn == asn1)
                contain_asn1 = true;

            else if(asn == asn2)
                contain_asn2 = true;
        }

        if(contain_asn1 != contain_asn2)
            return false;
    }

    return true;
}

// return a map of ASN to set of those it is indistinguishable from
std::unordered_map<uint32_t, std::set<uint32_t>> CommunityDetection::gen_ind_asn() {
    std::sort(sorted_asn_degree.begin(), sorted_asn_degree.end(), [](auto &a, auto &b) {
        // Sort from greatest degree to least
        return (*(a.second)) > (*(b.second));
    });

    std::unordered_map<uint32_t, std::set<uint32_t>> to_return;
    std::unordered_set<uint32_t> checked;

    for(size_t i = 0; i < sorted_asn_degree.size(); i++) {
        auto &pair = sorted_asn_degree[i];
        uint32_t cardinality = asn_to_cardinality.find(pair.first)->second;

        //until no AS with degree >= t + 1 remains
        if(*pair.second <= local_threshold)
            break;

        if(checked.find(pair.first) != checked.end())
            continue;
        
        std::set<uint32_t> to_add = { pair.first };

        for(size_t j = i + 1; j < sorted_asn_degree.size(); j++) {
            auto &other_pair = sorted_asn_degree.at(j);

            if(*other_pair.second != *pair.second)
                break;

            //If they have different cardinalities then it is not worth checking
            if(cardinality != asn_to_cardinality.find(other_pair.first)->second)
                continue;

            if(are_indistinguishable(pair.first, other_pair.first)) {
                to_add.insert(other_pair.first);
                checked.insert(other_pair.first);
            }
        }

        to_return.insert(std::make_pair(pair.first, to_add));
    }

    return to_return;

    // std::map<uint32_t, std::set<uint32_t>> retval;
    // for (auto iterator = asn_to_degree.begin(); iterator != asn_to_degree.end(); ++iterator) {
    //     //Make a set of all of the unique asns to eliminate that are indistinguishable from the AS "iterator" points to
    //     std::set<uint32_t> s_copy;
    //     for(auto copy_it = asn_to_degree.begin(); copy_it != asn_to_degree.end(); ++copy_it)
    //         s_copy.insert(copy_it->first);

    //     for (auto edge : hyper_edges) {
    //         if (std::find(edge.begin(), edge.end(), iterator->first) != edge.end()) {
    //             // if asn in edge, only save other ASNs also in the edge
    //             for (auto it = s_copy.begin(); it != s_copy.end() ;) {
    //                 if (*it != iterator->first && std::find(edge.begin(), edge.end(), *it) == edge.end()) {
    //                     // if asn missing from edge but present in s_copy, remove from s_copy
    //                     it = s_copy.erase(it);
    //                 } else { 
    //                     ++it;
    //                 }
    //             }
    //         } else {// If the asn is not in the list, remove everything that is there since they are not indistinguishable from the asn
    //             for(auto asn : edge)
    //                 s_copy.erase(asn);
    //         }
    //     }
    //     retval.insert(std::pair<uint32_t, std::set<uint32_t>>(iterator->first, s_copy));
    // }
    // return retval;
}

//Generate all of the subsets of nodes with length of local_threshold
//To be used to test MVC
bool CommunityDetection::is_suspect(std::vector<uint32_t> &candidate, std::vector<std::unordered_set<uint32_t>> &covers) {
    for(auto &cover : covers) {
        bool contains_candidate_node = false;

        for(auto asn : candidate) {
            if(std::find(cover.begin(), cover.end(), asn) != cover.end()) {
                contains_candidate_node = true;
                break;
            }
        }

        if(!contains_candidate_node)
            return false;
    }

    return true;
}

bool CommunityDetection::is_subset(std::vector<uint32_t> &super_set, std::vector<uint32_t> &candidate_subset) {
    for(auto asn : candidate_subset)
        if(std::find(super_set.begin(), super_set.end(), asn) == super_set.end())
            return false;
    return true;
}

void CommunityDetection::gen_suspect_candidates_helper_subset(std::vector<uint32_t> &current_subset, std::vector<std::unordered_set<uint32_t>> &covers, std::unordered_map<uint32_t, std::set<uint32_t>> &ind_map, std::vector<std::pair<uint32_t, std::shared_ptr<uint32_t>>> &distinguishable_subsets, std::vector<std::vector<uint32_t>> &results, int startIndex, int endIndex) {
    bool subset = false;
    for(auto &edge : hyper_edges) {
        if(is_subset(edge, current_subset)) {
            subset = true;
            break;
        }
    }
    
    if(subset && is_suspect(current_subset, covers)) {
        blacklist.insert(current_subset);
        return;
    }
    
    if(startIndex == endIndex)
        return;

    //Choose to include or exclude the current set
    for(auto asn : ind_map.at(distinguishable_subsets.at(startIndex).first))
        current_subset.push_back(asn);
    
    //Add this new subset to the total accumalation of subsets
    results.push_back(current_subset);
    gen_suspect_candidates_helper_subset(current_subset, covers, ind_map, distinguishable_subsets, results, startIndex + 1, endIndex);

    //Do not add building sum after this since this is what building sum originally was, which was already added in the previous recursive call
    for(size_t i = 0; i < ind_map.at(distinguishable_subsets.at(startIndex).first).size(); i++)
        current_subset.pop_back();
    gen_suspect_candidates_helper_subset(current_subset, covers, ind_map, distinguishable_subsets, results, startIndex + 1, endIndex);
}

void CommunityDetection::iterate_suspect_candidates_and_blacklist_helper(std::vector<uint32_t> &current, std::vector<std::unordered_set<uint32_t>> &covers, std::unordered_map<uint32_t, std::set<uint32_t>> &ind_map, 
                                                                            std::vector<std::pair<uint32_t, std::shared_ptr<uint32_t>>> &distinguishable_subsets, uint32_t distinguishable_index) {

    if(distinguishable_index >= distinguishable_subsets.size() - 1)
        return;

    uint32_t degree = *distinguishable_subsets.at(distinguishable_index).second;
    if(degree <= local_threshold)
        return;

    uint32_t end_same_degree_index = distinguishable_index + 1;
    for(; end_same_degree_index < distinguishable_subsets.size(); end_same_degree_index++)
        if(*distinguishable_subsets.at(end_same_degree_index).second != degree)
            break;
    
    std::vector<std::vector<uint32_t>> subsets_to_continue;

    gen_suspect_candidates_helper_subset(current, covers, ind_map, distinguishable_subsets, subsets_to_continue, distinguishable_index, end_same_degree_index);

    // for(auto &subset : subsets_to_append) {
    //     std::vector<uint32_t> current_copy = current;

    //     for(auto asn : subset)
    //         current_copy.push_back(asn);
        
    //     //If this is a suspect that should be blacklisted, don't bother finding its supersets
    //     if(is_suspect(current_copy))
    //         blacklist.insert(current_copy);
    //     else
        for(auto &subset_to_continue : subsets_to_continue)
            iterate_suspect_candidates_and_blacklist_helper(subset_to_continue, covers, ind_map, distinguishable_subsets, end_same_degree_index);
    // } 
}

// generate all possible cover candidates of size sz with given indistinguishability map
void CommunityDetection::iterate_suspect_candidates_and_blacklist(std::unordered_map<uint32_t, std::set<uint32_t>> &ind_map) {
    // auto distinguishable_subsets = sorted_asn_degree;
    // Indistinguishable nodes must be grouped together, so remove 'duplicates'
    // for (auto it = distinguishable_subsets.begin(); it != distinguishable_subsets.end(); ++it) {
        // for (auto it2 = it+1; it2 != distinguishable_subsets.end();) {
        //     if (std::find(ind_map[it->first].begin(), ind_map[it->first].end(), it2->first) != ind_map[it->first].end()) {
        //         it2 = distinguishable_subsets.erase(it2);
        //     } else {
        //         ++it2;
        //     }
        // }
    // }

    std::vector<std::pair<uint32_t, std::shared_ptr<uint32_t>>> distinguishable_subsets;
    for(auto &p : sorted_asn_degree)
        if(ind_map.find(p.first) != ind_map.end())
            distinguishable_subsets.push_back(p);

    auto covers = generate_covers();

    std::vector<uint32_t> running_sum;
    iterate_suspect_candidates_and_blacklist_helper(running_sum, covers, ind_map, distinguishable_subsets, 0);
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
        }
    }

    // These are not the MACs you are looking for....
    // In other words, while there are invalid MACs, they cannot be seen and are not between adopters. 
    if(hyper_edge_ranges.size() == 0)
        return; 

    for(std::pair<int, int> &range : hyper_edge_ranges) {
        // std::vector<uint32_t> hyper_edge(as_path.begin() + range.first, as_path.begin() + range.second + 1);

        //check if we have this edge already in the directory
        //When we go to add the edge later, we will check if we already have it in the total blacklisting list
        //no need to check twice

        // bool contains = false;
        // for(auto &temp_edge : edges_to_process) {
        //     if(std::equal(hyper_edge.begin(), hyper_edge.end(), temp_edge.begin())) {
        //         contains = true;
        //         break;
        //     }
        // }

        // if(contains)
        //     continue;

        edges_to_process.push_back(std::vector<uint32_t>(as_path.begin() + range.first, as_path.begin() + range.second + 1));
    }
}

void CommunityDetection::CD_algorithm() {
    // std::set<uint32_t> unique_asns = get_unique_asns(hyper_edges);
    auto ind_asn = gen_ind_asn();
    // std::map<uint32_t, uint32_t> degrees = get_degrees(unique_asns, hyper_edges);

    iterate_suspect_candidates_and_blacklist(ind_asn);

    //if there is any subset of S/S' with length t that makes a cover over the hyper edges, then S' is a suspect -> blacklist
    // for(auto &suspect : suspect_candidates) {
    //     auto unique_copy = unique_asns;

    //     for(auto asn : suspect)
    //         unique_copy.erase(asn);

    //     bool is_suspect = true;

    //     std::vector<std::unordered_set<uint32_t>> cover_candidates = generate_cover_candidates(unique_copy);
    //     for(auto &cover_candidate : cover_candidates) {
    //         if(is_cover(cover_candidate)) {
    //             is_suspect = false;
    //             break;
    //         }
    //     }

    //     if(is_suspect)
    //         blacklist.insert(suspect);
    // }
}

void CommunityDetection::local_threshold_approx_filtering_deprecated() {

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

    // Blacklist all the paths that were partially collapsed
    for (auto it = buckets.begin()+2; it != buckets.end(); ++it) {
        for (const auto &edge : *it) {
            blacklist.insert(edge);
        }
    }
}

void CommunityDetection::process_reports(EZASGraph *graph) {
    for(auto &edge : edges_to_process) {
        unsigned int reporter_policy = graph->ases->find(edge.at(0))->second->policy;

        if(reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_LOCAL || reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL || reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL_LOCAL) {
            add_hyper_edge(edge);
        }

        // Must be sorted for std::includes
        // (this is still correctly testing for a subset of the path)
        std::sort(edge.begin(), edge.end());
        blacklist.insert(std::move(edge));
    }

    edges_to_process.clear();

    if(hyper_edges.size() > 0) {
        CD_algorithm();
    }
}
