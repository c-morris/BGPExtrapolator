#include "CommunityDetection.h"

//**************** Node ****************//

CommunityDetection::Node::Node(uint32_t asn) : asn(asn) {

}

uint32_t CommunityDetection::Node::minimum_vertex_cover_helper(std::vector<std::vector<uint32_t>> hyper_edges_to_find, std::unordered_set<uint32_t> visited, Component *component) {
    if(hyper_edges_to_find.size() == 0)
        return 0;

    for(auto it = hyper_edges_to_find.begin(); it != hyper_edges_to_find.end(); ++it) {
        if(std::find(it->begin(), it->end(), asn) != it->end()) {
            hyper_edges_to_find.erase(it);
        }
    }

    visited.insert(asn);

    bool first_result = true;
    uint32_t min_result = 0;
    for(uint32_t neighbor : neighbors) {
        if(visited.find(neighbor) != visited.end())
            continue;

        uint32_t result = component->nodes.find(neighbor)->second->minimum_vertex_cover_helper(hyper_edges_to_find, visited, component);

        if(first_result) {
            first_result = false;
            min_result = result;
        } else if(result < min_result) {
            min_result = result;
        }
    }

    if(first_result)
        return 0;
    return 1 + min_result;
}

//**************** Component ****************//
CommunityDetection::Component::Component(std::vector<uint32_t> &hyper_edge) {
    unique_identifier = hyper_edge.at(0);
    add_hyper_edge(hyper_edge);
}

CommunityDetection::Component::~Component() {

}

void CommunityDetection::Component::remove_node(uint32_t asn) {
    auto node_search = nodes.find(asn);
    if(node_search == nodes.end())
        return;

    Node *node = node_search->second;
    for(uint32_t neighbor : node->neighbors) {
        Node *neighbor_node = nodes.find(neighbor)->second;
        neighbor_node->neighbors.erase(std::find(neighbor_node->neighbors.begin(), neighbor_node->neighbors.end(), asn));
    }

    delete node;
    nodes.erase(node_search);
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

    if(degree_count == 0)
        remove_node(asn);

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
    Logger::getInstance().log("Debug") << "Hyper edge: " << hyper_edge << " is being added to the hyper graph";

    hyper_edges.push_back(hyper_edge);
    for(size_t i = 0; i < hyper_edge.size(); i++) {
        change_degree(hyper_edge.at(i), true);

        auto node_search = nodes.find(hyper_edge.at(i));
        if(node_search == nodes.end())
            nodes.insert(std::make_pair(hyper_edge.at(i), new Node(hyper_edge.at(i))));

        if(i > 0) {
            nodes.find(hyper_edge.at(i - 1))->second->neighbors.push_back(hyper_edge.at(i));
            nodes.find(hyper_edge.at(i))->second->neighbors.push_back(hyper_edge.at(i - 1));
        }
    }
}

void CommunityDetection::Component::merge(Component *other) {
    std::vector<std::vector<uint32_t>> &other_hyper_edges = other->hyper_edges;

    for(std::vector<uint32_t> &hyper_edge : other_hyper_edges)
        add_hyper_edge(hyper_edge);
}

uint32_t CommunityDetection::Component::minimum_vertex_cover(uint32_t asn) {
    std::vector<std::vector<uint32_t>> hyper_edges_to_find;

    for(auto &edge : this->hyper_edges)
        if(std::find(edge.begin(), edge.end(), asn) != edge.end())
            hyper_edges_to_find.push_back(edge);

    auto node_search = nodes.find(asn);
    if(node_search == nodes.end())
        return 0;
    
    // -1 to get rid of the +1 from the root node adding to the findings of its neighbors  
    return node_search->second->minimum_vertex_cover_helper(hyper_edges_to_find, std::unordered_set<uint32_t>(), this) - 1;
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

void CommunityDetection::Component::threshold_filtering(CommunityDetection *community_detection, EZASGraph *graph) {
    if(community_detection->threshold == 0)
        return;

    bool changed = true;
    while(changed && community_detection->threshold > 0) {
        //find the highest degree in the graph
        uint32_t highest_mvc = 0;
        uint32_t highest_mvc_asn = 0;
        for(auto pair : nodes) {
            uint32_t mvc = minimum_vertex_cover(pair.first);

            if(mvc > highest_mvc) {
                highest_mvc = mvc;
                highest_mvc_asn = pair.first;
            }
        }

        //if there is an AS with an mvc higher than the threshold, remove the AS and decrement threshold
        //Check that it is also greater than or equal to 2 (don't remove something with an mvc of 1)
        if(highest_mvc >= 2 && highest_mvc >= community_detection->threshold)
            remove_AS(highest_mvc_asn);
        else
            changed = false;
    }
}

void CommunityDetection::Component::virtual_pair_removal(CommunityDetection *community_detection, EZASGraph *graph) {
    threshold_filtering(community_detection, graph);

    //For all removal sequences...
    for(size_t i = 0; i < hyper_edges.size(); i++) {
        CommunityDetection temp_community_detection(community_detection->threshold - 1);

        for(size_t j = 0; j < hyper_edges.size(); j++) {
            if(i == j) continue;
            temp_community_detection.add_hyper_edge(hyper_edges.at(j));
        }

        //Do community detection on that 
        temp_community_detection.virtual_pair_removal(graph);
    }
}

//**************** Community Detection ****************//

CommunityDetection::CommunityDetection(uint32_t threshold) : threshold(threshold) { }
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

            Logger::getInstance().log("Debug") << "Merging components " << first->unique_identifier << " and " << to_merge->unique_identifier; 

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
    if(!reporting_as->adopter)
        return;

    //******   Trim Report to Hyper Edges   ******//
    //End of the path is the origin
    //The condition of an invalid MAC is this: 
    //    When an adopter at index i is not a true neighbor of the AS at i - 1.
    //Consider an adopting origin, which is at the very end of the path
    //There will be an invalid MAC if the neighbor on the path at the index length - 1 is not a real neighbor

    //This will work by starting at the 0 index of the list and keeping track of the last adopter
    //We need to see if the invalid MAC is between two adopters
    //However, if we move over a non-adopter or find an invalid MAC
    //We must remeber the last trusted adopter

    // A - adopter. N - Non adopter. O - Origin, who is an adopter
    //Suppose the invalid MAC is between the origin and the non adopter
    
    //A A N N O
    //^

    //A A N N O
    //  ^

    //A A N N O
    //  * ^

    //A A N N O
    //  *   ^ 

    //A A N N O
    //  *     ^  <-- Invalid MAC found

    //This will return the path {A N N O}, for whatever asns they may be, and how it trimmed out that first adopter
    //It is safe to trim out this adopter since it had a valid MAC for a key it did not have, and thus could not fake this report.
    //This is because the algorithm remembered what adopter that was last validated (represented as *)
    //Notice that the * did not move over the non adopters
    //This must also generalize to having multiple hyper edges on the path

    //Indecies of the hyper edge range
    std::vector<std::pair<int, int>> hyper_edge_ranges;

    //This is true because the reporting AS must be an adopter and the reporting AS is at index 0.
    int last_adopter_index = 0;

    //Turn true when there is an invalid MAC or non-adopter
    //This is essentially the control of where to place * from the example above.
    //We will keep track of the adopters until we hit a non adopter or invalid MAC
    bool stop_tracking_adopter = false;

    for(size_t i = 1; i < as_path.size(); i++) {// Starts at 1 because we need to check if the "previous" AS is a real neighbor
        uint32_t asn = as_path.at(i);
        auto as_search = graph->ases->find(asn);
        if(as_search == graph->ases->end()) {
            std::cerr << "Non-existent AS on the path!" << std::endl;
            return;
        }

        EZAS *as = as_search->second;

        //Can't track over a non-adopter
        if(!as->adopter) {
            stop_tracking_adopter = true;
        }

        //Found invalid MAC
        if(as->adopter && !as->has_neighbor(as_path.at(i - 1))) {
            //Add to ranges that will construct the hyper edges from this report
            hyper_edge_ranges.push_back(std::pair<int, int>(last_adopter_index, i));

            stop_tracking_adopter = false;

            //TODO, do we trust this adopter?
            last_adopter_index = i;
        } else if(!stop_tracking_adopter) {
            last_adopter_index++;
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
        add_hyper_edge(hyper_edge);
    }
}

void CommunityDetection::threshold_filtering(EZASGraph *graph) {
    for(auto &p : identifier_to_component)
        p.second->threshold_filtering(this, graph);
}

void CommunityDetection::virtual_pair_removal(EZASGraph *graph) {
    if(threshold == 0)
        return;

    for(auto &p : identifier_to_component)
        p.second->virtual_pair_removal(this, graph);
}