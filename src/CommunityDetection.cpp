#include <limits.h>

#include "CommunityDetection.h"

//**************** Component ****************//
CommunityDetection::Component::Component(std::vector<uint32_t> &hyper_edge) {
    //TODO: This will break if an AS is removed and then added to another component
    unique_identifier = hyper_edge.at(0);
    add_hyper_edge(hyper_edge);
}

CommunityDetection::Component::~Component() {

}

bool CommunityDetection::Component::contains_hyper_edge(std::vector<uint32_t> &hyper_edge) {
    for(auto &temp_edge : hyper_edges)
        if(std::equal(hyper_edge.begin(), hyper_edge.end(), temp_edge.begin()))
            return true;
    return false;
}

void CommunityDetection::Component::change_degree(uint32_t asn, bool increment) {
    auto initial_degree_count_search = as_to_degree_count.find(asn);

    uint32_t degree_count = 0;
    if(initial_degree_count_search != as_to_degree_count.end()) {
        degree_count = initial_degree_count_search->second;
        as_to_degree_count.erase(initial_degree_count_search);
    }

    auto initial_degree_set_search  = degree_sets.find(degree_count);
    if(initial_degree_set_search != degree_sets.end()) {
        std::set<uint32_t> &degree_set = initial_degree_set_search->second;
        degree_set.erase(asn);

        if(degree_set.size() == 0)
            degree_sets.erase(initial_degree_set_search);
    }

    if(increment)
        degree_count++;
    else
        degree_count--;

    if(degree_count > 0) {
        as_to_degree_count.insert(std::make_pair(asn, degree_count));

        auto degree_set_search = degree_sets.find(degree_count);
        if(degree_set_search == degree_sets.end()) {
            auto it = degree_sets.insert(std::make_pair(degree_count, std::set<uint32_t>()));
            std::set<uint32_t> &final_degree_set = (*it.first).second;

            final_degree_set.insert(asn);
        } else {
            std::set<uint32_t> &final_degree_set = degree_set_search->second;
            final_degree_set.insert(asn);
        }
    }
}

void CommunityDetection::Component::add_hyper_edge(std::vector<uint32_t> &hyper_edge) {
    //Logger::getInstance().log("Debug") << "Hyper edge: " << hyper_edge << " is being added to the hyper graph";

    hyper_edges.push_back(hyper_edge);
    for(size_t i = 0; i < hyper_edge.size(); i++)
        change_degree(hyper_edge.at(i), true);
}

void CommunityDetection::Component::merge(Component *other) {
    std::vector<std::vector<uint32_t>> &other_hyper_edges = other->hyper_edges;

    for(std::vector<uint32_t> &hyper_edge : other_hyper_edges)
        add_hyper_edge(hyper_edge);
}

uint32_t CommunityDetection::Component::minimum_vertex_cover_helper(uint32_t root_asn, std::vector<std::vector<uint32_t>> hyper_edges_to_find, bool local, uint32_t thresh, uint32_t best_so_far, uint32_t depth) {
    if(hyper_edges_to_find.size() == 0)
        return 0;
    // Stop if a better solution has already been found OR if the size of this cover exceeds thresh
    if (depth > best_so_far or depth > thresh)
        return 1;

    std::unordered_set<uint32_t> asns_to_attempt;
    for(auto &edge : hyper_edges_to_find) {
        for(size_t i = 0; i < edge.size(); i++) {
            if(local) {//Trial remove only neighbors and not the root
                if(edge.at(i) != root_asn)
                    continue;

                if(i != 0)
                    asns_to_attempt.insert(edge.at(i - 1));
                else if(i != edge.size() - 1)
                    asns_to_attempt.insert(edge.at(i + 1));
            } else {//Trial remove all but the root AS
                if(edge.at(i) == root_asn)
                    continue;

                asns_to_attempt.insert(edge.at(i));
            }
        }
    }
    
    uint32_t best_result = UINT_MAX;
    for(uint32_t asn : asns_to_attempt) {
        std::vector<std::vector<uint32_t>> next_hyper_edges_to_find;

        for(auto edge : hyper_edges_to_find) {
            //Eliminate any edge containing this ASN
            if(std::find(edge.begin(), edge.end(), asn) == edge.end())
                next_hyper_edges_to_find.push_back(edge);
        }

        uint32_t result = minimum_vertex_cover_helper(root_asn, next_hyper_edges_to_find, local, thresh, best_so_far, depth+1);

        if(result+depth < best_so_far) {
            // If we got a smaller cover than we have seen before, update the best so far
            best_so_far = result+depth;
        }
        // If one branch of this function call tree terminates, no sense in going deeper, it can only get worse
        if (result == 0) {
            return 1;
        }
        if(result < best_result) {
            best_result = result;
        }
    }

    return 1 + best_result;
}

uint32_t CommunityDetection::Component::minimum_vertex_cover(uint32_t asn, uint32_t thresh) {
    std::vector<std::vector<uint32_t>> hyper_edges_to_find;

    //TODO we need a data structure to supply the hyper edges that an AS is in
    for(auto &edge : this->hyper_edges)
        if(std::find(edge.begin(), edge.end(), asn) != edge.end())
            hyper_edges_to_find.push_back(edge);

    return minimum_vertex_cover_helper(asn, hyper_edges_to_find, false, thresh, UINT_MAX, 0);
}

uint32_t CommunityDetection::Component::local_minimum_vertex_cover(uint32_t asn, uint32_t thresh) {
    std::vector<std::vector<uint32_t>> hyper_edges_to_find;

    for(auto &edge : this->hyper_edges)
        if(std::find(edge.begin(), edge.end(), asn) != edge.end())
            hyper_edges_to_find.push_back(edge);

    std::cout << "About to start local CD with " << hyper_edges_to_find.size() << " edges..." << std::endl;

    return minimum_vertex_cover_helper(asn, hyper_edges_to_find, true, thresh, UINT_MAX, 0);
}

void CommunityDetection::Component::remove_hyper_edge(std::vector<uint32_t> &hyper_edge) {
    auto it = std::find(hyper_edges.begin(), hyper_edges.end(), hyper_edge);

    if(it == hyper_edges.end())
        return;

    for(uint32_t asn : hyper_edge)
        change_degree(asn, false);
    
    hyper_edges.erase(it);
}

void CommunityDetection::Component::remove_AS(uint32_t asn_to_remove) {
    //Erase hyper edges containing this AS
    for(auto it = hyper_edges.begin(); it != hyper_edges.end(); ++it)
        if(std::find(it->begin(), it->end(), asn_to_remove) != it->end())
            remove_hyper_edge(*it);
}

void CommunityDetection::Component::local_threshold_filtering(CommunityDetection *community_detection) {
    for(auto &pair : as_to_degree_count) {
        if(local_minimum_vertex_cover(pair.first, community_detection->local_threshold) > community_detection->local_threshold)
            community_detection->blacklist_asns.insert(pair.first);
    }
}

void CommunityDetection::Component::global_threshold_filtering(CommunityDetection *community_detection) {
    if(community_detection->global_threshold == 0)
        return;

    bool changed = true;
    while(changed && community_detection->global_threshold > 0) {
        //find the highest degree in the graph
        uint32_t highest_mvc = 0;
        uint32_t highest_mvc_asn = 0;
        for(auto &pair : as_to_degree_count) {
            uint32_t mvc = minimum_vertex_cover(pair.first, community_detection->global_threshold);

            if(mvc > highest_mvc) {
                highest_mvc = mvc;
                highest_mvc_asn = pair.first;
            }
        }

        //if there is an AS with an mvc higher than the threshold, blacklist the AS and decrement threshold
        //Check that it is also greater than or equal to 2 (don't remove something with an mvc of 1)
        if(highest_mvc > community_detection->global_threshold && highest_mvc > 1) {
            community_detection->blacklist_asns.insert(highest_mvc_asn);
            community_detection->global_threshold--;
        } else {
            changed = false;
        }
    }
}

/**
 * I do not believe this is right? We need test cases for virtual pair removal to see how this should work 
 */
void CommunityDetection::Component::virtual_pair_removal(CommunityDetection *community_detection) {
    global_threshold_filtering(community_detection);

    //For all removal sequences...
    for(size_t i = 0; i < hyper_edges.size(); i++) {
        CommunityDetection temp_community_detection(community_detection->global_threshold - 1);

        for(size_t j = 0; j < hyper_edges.size(); j++) {
            if(i == j) continue;
            temp_community_detection.add_hyper_edge(hyper_edges.at(j));
        }

        //Do community detection on that 
        temp_community_detection.virtual_pair_removal();
    }
}

//**************** Community Detection ****************//

CommunityDetection::CommunityDetection(uint32_t local_threshold) : local_threshold(local_threshold) { }
CommunityDetection::~CommunityDetection() {
    clear();
}

void CommunityDetection::clear() {
    for(auto &pair : identifier_to_component)
        delete pair.second;
    identifier_to_component.clear();
}

bool CommunityDetection::contains_hyper_edge(std::vector<uint32_t> &hyper_edge) {
    for(auto &pair : identifier_to_component)
        if(pair.second->contains_hyper_edge(hyper_edge))
            return true;
    return false;
}

void CommunityDetection::add_hyper_edge(std::vector<uint32_t> &hyper_edge) {
    if(contains_hyper_edge(hyper_edge))
        return;

    //Go through the reporting path and see what components these ASes belong to in the graph
    //We will record the unique identifier to the components that these ASes already belong to
    std::unordered_set<uint32_t> components_to_merge;
    for(uint32_t asn : hyper_edge) {
        for(auto &pair : identifier_to_component) {
            Component *component = pair.second;

            auto as_search = component->as_to_degree_count.find(asn);

            if(as_search != component->as_to_degree_count.end())
                components_to_merge.insert(component->unique_identifier);
        }
    }

    if(components_to_merge.size() == 0) {//Need to add a component for these ASes to be added to
        identifier_to_component.insert(std::make_pair(hyper_edge.at(0), new Component(hyper_edge)));
    } else if(components_to_merge.size() == 1) {//Add the ASes to the only component any of them are in
        identifier_to_component.find((*(components_to_merge.begin()++)))->second->add_hyper_edge(hyper_edge);
    } else {//An AS cannot be in more than one component. So if this report connects two or more components, then they need to merge
        //What is going to happen here is that the first component will absorb all of the information from the rest of the components
        auto iterator = components_to_merge.begin();
        Component *first = identifier_to_component.find(*iterator)->second;

        //For all components, other than the first of course hece the ;, add all information to the first component
        //Then delete the component since its information has been recorded
        iterator++;
        while(iterator != components_to_merge.end()) {
            auto component_search_result = identifier_to_component.find(*iterator);

            Component *to_merge = component_search_result->second;

            first->merge(to_merge);
            
            delete to_merge;
            identifier_to_component.erase(component_search_result);

            iterator++;
        }

        //Add in the hyper edge that caused all of this nonsense
        first->add_hyper_edge(hyper_edge);
    }
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

    //There is an invalid MAC, keep an eye out for an adopter on the other side of the invalid MAC
    bool search_for_adopter = false;

    for(int i = as_path.size() - 2; i >= 0; i--) {
        auto current_search = graph->ases->find(as_path.at(i));
        if(current_search == graph->ases->end()) {
            return;
        }

        auto previous_search = graph->ases->find(as_path.at(i + 1));
        if(previous_search == graph->ases->end()) {
            return;
        }

        EZAS *current_as = current_search->second;
        EZAS *previous_as = previous_search->second;

        //Previous AS is an adopter and does not have an actual relationship to this AS (invalid MAC)
        if(previous_as->policy != EZAS_TYPE_BGP && !previous_as->has_neighbor(current_as->asn)) {
            as_index_invalid_relationship = i + 1;
            search_for_adopter = true;
            continue;
        }

        if(current_as->policy != EZAS_TYPE_BGP && search_for_adopter) {
            search_for_adopter = false;
            hyper_edge_ranges.push_back(std::make_pair(i, as_index_invalid_relationship));
        }
    }

    // These are not the MACs you are looking for....
    // In other words, while there are invalid MACs, they cannot be seen and are not between adopters. 
    if(hyper_edge_ranges.size() == 0)
        return; 

    for(std::pair<int, int> &range : hyper_edge_ranges) {
        std::vector<uint32_t> hyper_edge;
        for(int i = range.first; i <= range.second; i++)
            hyper_edge.push_back(as_path.at(i));

        //Add the hyper edge to the component it belongs to (ASes on the path belong to no existing component and thos that do all belong to the same component)
        //Or merge components if the path contains more than one AS that belong to different components already (an AS can be in 1 component only)
        //Or create a whole new component if none of the ASes belong to an existing component
        // add_hyper_edge(hyper_edge);
        
        bool contains = false;
        for(auto &temp_edge : edges_to_proccess) {
            if(std::equal(hyper_edge.begin(), hyper_edge.end(), temp_edge.begin())) {
                contains = true;
                break;
            }
        }

        if(contains)
            continue;

        edges_to_proccess.push_back(hyper_edge);
    }
}

void CommunityDetection::local_threshold_filtering() {
    for(auto &p : identifier_to_component)
        p.second->local_threshold_filtering(this);
}

void CommunityDetection::global_threshold_filtering() {
    for(auto &p : identifier_to_component)
        p.second->global_threshold_filtering(this);
}

void CommunityDetection::virtual_pair_removal() {
    if(global_threshold == 0)
        return;

    for(auto &p : identifier_to_component)
        p.second->virtual_pair_removal(this);
}

void CommunityDetection::process_reports(EZASGraph *graph) {
    for(auto &edge : edges_to_proccess) {
        /**
         * Attacker directly in between two adopters. This edge only got this far if it went through add_report which checks for *visible* invalid MACs 
         * 
         * If we want a mix of policies, this may need to change since a CD adopter and a directory only adopter next to each other changes this
         * 
         * add_report narrows the path down to the two adopters with the invalid MAC in between. However, if an adopter is directory only then it won't send to CD
         * however, a neighbor to the directory only adopter will, but that neighbor is not considered by this algorithm since it assumes no mix of policies
         */
        if(edge.size() == 3) {
            blacklist_asns.insert(edge.at(1));
        }

        blacklist_paths.push_back(edge);

        unsigned int reporter_policy = graph->ases->find(edge.at(edge.size() - 1))->second->policy;

        if(reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_LOCAL || reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL || reporter_policy == EZAS_TYPE_COMMUNITY_DETECTION_GLOBAL_LOCAL) {
            add_hyper_edge(edge);
        }
    }

    local_threshold_filtering();
    edges_to_proccess.clear();
}
