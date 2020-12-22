#include "Tests/Tests.h"
#include "Extrapolators/EZExtrapolator.h"

/** Test path seeding the graph with announcements from monitors. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 */
bool ezbgpsec_test_path_propagation() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    EZAnnouncement attack_announcement(5, p.addr, p.netmask, 300, 5, 2, false, true);

    e.graph->ases->find(5)->second->process_announcement(attack_announcement);

    e.propagate_up();
    e.propagate_down();

    if(e.graph->ases->find(1)->second->all_anns->find(p)->second.as_path.at(0) != 1 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->second.as_path.at(1) != 2 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->second.as_path.at(2) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #1 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(2)->second->all_anns->find(p)->second.as_path.at(0) != 2 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->second.as_path.at(1) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #2 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(3)->second->all_anns->find(p)->second.as_path.at(0) != 3 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.as_path.at(1) != 2 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->second.as_path.at(2) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #3 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(4)->second->all_anns->find(p)->second.as_path.at(0) != 4 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->second.as_path.at(1) != 2 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->second.as_path.at(2) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #4 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(5)->second->all_anns->find(p)->second.as_path.at(0) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #5 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(6)->second->all_anns->find(p)->second.as_path.at(0) != 6 ||
        e.graph->ases->find(6)->second->all_anns->find(p)->second.as_path.at(1) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #2 full path incorrect!" << std::endl;
        return false;
    }

    return true;
}

/** Test if components can be added to the CD graph
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *  3 - origin
 *  5 - attacker
 *  4 - victim
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 */
bool ezbgpsec_test_gather_reports() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    //2 is the first so it will be the first to report
    e.graph->adopters->push_back(2);
    e.graph->adopters->push_back(4);
    e.graph->adopters->push_back(3);
    // e.graph->adopters->push_back(1);

    //Set them to locally know that they are an adopter
    for(uint32_t asn : *e.graph->adopters) {
        e.graph->ases->find(asn)->second->adopter = true;
    }

    // e.graph->adopters->push_back(6);

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");

    //4 being the victim doesn't really matter, just need the prefix to be in here
    e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(4, p));

    //Make an announcement that states 5 and 3 are neighbors. This will create an invalid MAC
    EZAnnouncement attack_announcement(3, p.addr, p.netmask, 299, 3, 2, false, true);
    //The supposed path thus far
    attack_announcement.as_path.push_back(3);

    e.graph->ases->find(5)->second->process_announcement(attack_announcement);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();

    CommunityDetection *community_detection = e.communityDetection;
    if(community_detection->identifier_to_component.size() != 1) {
        std::cerr << "Component was not created properly!" << std::endl;
        return false;
    }

    //2 should be the identifier since it will be the first adopter to report the invalid MAC
    auto compnent_search = community_detection->identifier_to_component.find(2);
    if(compnent_search == community_detection->identifier_to_component.end()) {
        std::cerr << "Unique identifier is not correct!" << std::endl;
        return false;
    }

    CommunityDetection::Component *component = compnent_search->second;
    if(component->hyper_edges.size() != 1) {
        std::cerr << "The hyper edge was not added to the component properly!" << std::endl;
        return false;
    }

    //Check degree count and existance of AS 3
    auto three_search = component->as_to_degree_count.find(3);
    if(three_search == component->as_to_degree_count.end()) {
        std::cerr << "AS 3 is not in the component!" << std::endl;
        return false;
    }

    if(three_search->second != 1) {
        std::cerr << "AS 3 has the wrong degree count!" << std::endl;
        return false;
    }

    ///Check degree count and existance of AS 5
    auto five_search = component->as_to_degree_count.find(5);
    if(five_search == component->as_to_degree_count.end()) {
        std::cerr << "AS 5 is not in the component!" << std::endl;
        return false;
    }

    if(five_search->second != 1) {
        std::cerr << "AS 5 has the wrong degree count!" << std::endl;
        return false;
    }

    //Check degree count and existance of AS 2
    auto two_search = component->as_to_degree_count.find(2);
    if(two_search == component->as_to_degree_count.end()) {
        std::cerr << "AS 2 is not in the component!" << std::endl;
        return false;
    }

    if(two_search->second != 1) {
        std::cerr << "AS 2 has the wrong degree count!" << std::endl;
        return false;
    }

    //All of the ASes with degree count of 1
    auto degree_set_search = component->degree_sets.find(1);
    if(degree_set_search == component->degree_sets.end()) {
        std::cerr << "Degree Set was not created for the first announcement!" << std::endl;
        return false;
    }

    std::set<uint32_t> degree_set = degree_set_search->second;
    if(degree_set.size() != 3) {
        std::cerr << "Degree Set does not have just the 3 ASes for the first announcement!" << std::endl;
        return false;
    }

    if(degree_set.find(2) == degree_set.end() || degree_set.find(3) == degree_set.end() || degree_set.find(5) == degree_set.end()) {
        std::cerr << "Degree Set has the wrong ASes in it for the first announcement!" << std::endl;
        return false; 
    }

    Prefix<> p2 = Prefix<>("1.1.0.0", "255.255.0.0");

    //Prefixes need not compete for 2's favor, thus they both need to be checked
    e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(6, p2));

    EZAnnouncement attack_announcement2(3, p2.addr, p2.netmask, 299, 3, 2, false, true);
    attack_announcement2.as_path.push_back(3);//Make the origin on the path

    e.graph->ases->find(1)->second->process_announcement(attack_announcement2);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();

    if(community_detection->identifier_to_component.size() != 1) {
        std::cerr << "Component was not created properly after the second announcement!" << std::endl;
        return false;
    }

    //2 should be the identifier since it will be the first adopter to report the invalid MAC
    auto compnent_search2 = community_detection->identifier_to_component.find(2);
    if(compnent_search2 == community_detection->identifier_to_component.end()) {
        std::cerr << "Unique identifier is not correct after the second announcement!" << std::endl;
        return false;
    }

    if(component->hyper_edges.size() != 2) {
        std::cerr << "The hyper edge was not added to the component properly after the second announcement!" << std::endl;
        return false;
    }

    //Check degree count and existance of AS 3
    auto three_search2 = component->as_to_degree_count.find(3);
    if(three_search2 == component->as_to_degree_count.end()) {
        std::cerr << "AS 3 is not in the component after the second announcement!" << std::endl;
        return false;
    }

    if(three_search2->second != 2) {
        std::cerr << "AS 3 has the wrong degree count after second announcement!" << std::endl;
        return false;
    }

    ///Check degree count and existance of AS 5
    auto five_search2 = component->as_to_degree_count.find(5);
    if(five_search2 == component->as_to_degree_count.end()) {
        std::cerr << "AS 5 is not in the component after the second announcement!" << std::endl;
        return false;
    }

    if(five_search2->second != 1) {
        std::cerr << "AS 5 has the wrong degree count after the second announcement!" << std::endl;
        return false;
    }

    ///Check degree count and existance of AS 5
    auto one_search = component->as_to_degree_count.find(1);
    if(one_search == component->as_to_degree_count.end()) {
        std::cerr << "AS 1 is not in the component after the second announcement!" << std::endl;
        return false;
    }

    if(one_search->second != 1) {
        std::cerr << "AS 1 has the wrong degree count after the second announcement!" << std::endl;
        return false;
    }

    //Check degree count and existance of AS 2
    auto two_search2 = component->as_to_degree_count.find(2);
    if(two_search2 == component->as_to_degree_count.end()) {
        std::cerr << "AS 2 is not in the component after the second announcement!" << std::endl;
        return false;
    }

    if(two_search2->second != 2) {
        std::cerr << "AS 2 has the wrong degree count after the second announcement!" << std::endl;
        return false;
    }

    //All of the ASes with degree count of 1
    auto degree_set_search1 = component->degree_sets.find(1);
    if(degree_set_search1 == component->degree_sets.end()) {
        std::cerr << "Degree Set 1 was not created for the second announcement!" << std::endl;
        return false;
    }

    std::set<uint32_t> degree_set1 = degree_set_search1->second;
    if(degree_set1.size() != 2) {
        std::cerr << "Degree Set 1 does not have just the 2 ASes for the second announcement!" << std::endl;
        return false;
    }

    if(degree_set1.find(1) == degree_set1.end() || degree_set1.find(5) == degree_set1.end()) {
        std::cerr << "Degree Set 1 has the wrong ASes in it for the second announcement!" << std::endl;
        return false; 
    }

    //All of the ASes with degree count of 2
    auto degree_set_search2 = component->degree_sets.find(2);
    if(degree_set_search2 == component->degree_sets.end()) {
        std::cerr << "Degree Set 2 was not created for the second announcement!" << std::endl;
        return false;
    }

    std::set<uint32_t> degree_set2 = degree_set_search2->second;
    if(degree_set2.size() != 2) {
        std::cerr << "Degree Set 2 does not have just the 2 ASes for the second announcement!" << std::endl;
        return false;
    }

    if(degree_set2.find(3) == degree_set2.end() || degree_set2.find(2) == degree_set2.end()) {
        std::cerr << "Degree Set 2 has the wrong ASes in it for the second announcement!" << std::endl;
        return false; 
    }

    return true;
}

/** Test if components can be merged in the CD graph
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 *  
 *  Non-adopters/Attackers: 1 and 4
 * 
 *  1   4
 *  |   |
 *  2   5
 *  |   |
 *  3   6
 * 
 * Attacks will be done by 1 and 4, claiming to be neighbors to 3 and 6 respectively. Then, 1 will claim to be a neighbor to 5. 
 * This will merge the components IN THE HYPERGRAPH.
 */
bool ezbgpsec_test_gather_reports_merge() {
    // Logger::setFolder("./Logs/");

    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);

    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);

    e.graph->add_relationship(5, 4, AS_REL_PROVIDER);
    e.graph->add_relationship(4, 5, AS_REL_CUSTOMER);

    e.graph->add_relationship(6, 5, AS_REL_PROVIDER);
    e.graph->add_relationship(5, 6, AS_REL_CUSTOMER);

    e.graph->decide_ranks();

    //2 is the first so it will be the first to report
    e.graph->adopters->push_back(2);
    e.graph->adopters->push_back(3);

    e.graph->adopters->push_back(5);
    e.graph->adopters->push_back(6);

    //Set them to locally know that they are an adopter
    for(uint32_t asn : *e.graph->adopters) {
        e.graph->ases->find(asn)->second->adopter = true;
    }

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");

    //4 being the victim doesn't really matter, just need the prefix to be in here
    e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(4, p));

    //Make an announcement that states 5 and 3 are neighbors. This will create an invalid MAC
    EZAnnouncement attack_announcement(3, p.addr, p.netmask, 299, 3, 2, false, true);
    //The supposed path thus far
    attack_announcement.as_path.push_back(3);

    e.graph->ases->find(1)->second->process_announcement(attack_announcement);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();

    CommunityDetection *community_detection = e.communityDetection;
    if(community_detection->identifier_to_component.size() != 1) {
        std::cerr << "Component was not created properly!" << std::endl;
        return false;
    }

    //2 should be the identifier since it will be the first adopter to report the invalid MAC
    auto compnent_search = community_detection->identifier_to_component.find(2);
    if(compnent_search == community_detection->identifier_to_component.end()) {
        std::cerr << "Unique identifier is not correct!" << std::endl;
        return false;
    }

    CommunityDetection::Component *component = compnent_search->second;
    if(component->hyper_edges.size() != 1) {
        std::cerr << "The hyper edge was not added to the component properly!" << std::endl;
        return false;
    }

    //Check degree count and existance of AS 3
    auto three_search = component->as_to_degree_count.find(3);
    if(three_search == component->as_to_degree_count.end()) {
        std::cerr << "AS 3 is not in the component!" << std::endl;
        return false;
    }

    if(three_search->second != 1) {
        std::cerr << "AS 3 has the wrong degree count!" << std::endl;
        return false;
    }

    ///Check degree count and existance of AS 1
    auto one_search = component->as_to_degree_count.find(1);
    if(one_search == component->as_to_degree_count.end()) {
        std::cerr << "AS 1 is not in the component!" << std::endl;
        return false;
    }

    if(one_search->second != 1) {
        std::cerr << "AS 1 has the wrong degree count!" << std::endl;
        return false;
    }

    //Check degree count and existance of AS 2
    auto two_search = component->as_to_degree_count.find(2);
    if(two_search == component->as_to_degree_count.end()) {
        std::cerr << "AS 2 is not in the component!" << std::endl;
        return false;
    }

    if(two_search->second != 1) {
        std::cerr << "AS 2 has the wrong degree count!" << std::endl;
        return false;
    }

    //All of the ASes with degree count of 1
    auto degree_set_search = component->degree_sets.find(1);
    if(degree_set_search == component->degree_sets.end()) {
        std::cerr << "Degree Set was not created for the first announcement!" << std::endl;
        return false;
    }

    std::set<uint32_t> degree_set = degree_set_search->second;
    if(degree_set.size() != 3) {
        std::cerr << "Degree Set does not have just the 3 ASes for the first announcement!" << std::endl;
        return false;
    }

    if(degree_set.find(1) == degree_set.end() || degree_set.find(2) == degree_set.end() || degree_set.find(3) == degree_set.end()) {
        std::cerr << "Degree Set has the wrong ASes in it for the first announcement!" << std::endl;
        return false; 
    }

    Prefix<> p2 = Prefix<>("1.1.0.0", "255.255.0.0");

    //Prefixes need not compete for 2's favor, thus they both need to be checked
    e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(6, p2));

    EZAnnouncement attack_announcement2(6, p2.addr, p2.netmask, 299, 6, 2, false, true);
    attack_announcement2.as_path.push_back(6);//Make the origin on the path

    e.graph->ases->find(4)->second->process_announcement(attack_announcement2);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();

    if(community_detection->identifier_to_component.size() != 2) {
        std::cerr << "Second component was not created properly after the second announcement!" << std::endl;
        return false;
    }

    //5 should be the identifier since it will be the first adopter to report the invalid MAC
    auto component_search2 = community_detection->identifier_to_component.find(5);
    if(component_search2 == community_detection->identifier_to_component.end()) {
        std::cerr << "Unique identifier is not correct after the second announcement!" << std::endl;
        return false;
    }

    CommunityDetection::Component *component2 = component_search2->second;
    if(component2->hyper_edges.size() != 1) {
        std::cerr << "The hyper edge was not added to the second component properly after the second announcement!" << std::endl;
        return false;
    }

    //All should have 1 degree
    for(int i = 4; i <= 6; i++) {
        //Check degree count and existance of AS i
        auto search2 = component2->as_to_degree_count.find(i);
        if(search2 == component2->as_to_degree_count.end()) {
            std::cerr << "AS " << i << " is not in the component after the second announcement!" << std::endl;
            return false;
        }

        if(search2->second != 1) {
            std::cerr << "AS " << i << " has the wrong degree count after second announcement!" << std::endl;
            return false;
        }
    }

    //All of the ASes with degree count of 1
    auto degree_set_search1 = component2->degree_sets.find(1);
    if(degree_set_search1 == component2->degree_sets.end()) {
        std::cerr << "Degree Set 1 was not created for the second announcement!" << std::endl;
        return false;
    }

    std::set<uint32_t> degree_set1 = degree_set_search1->second;
    if(degree_set1.size() != 3) {
        std::cerr << "Degree Set 1 does not have just the 2 ASes for the second announcement!" << std::endl;
        return false;
    }

    if(degree_set1.find(4) == degree_set1.end() || degree_set1.find(5) == degree_set1.end() || degree_set1.find(6) == degree_set1.end()) {
        std::cerr << "Degree Set 1 has the wrong ASes in it for the second announcement!" << std::endl;
        return false; 
    }

    //MERGE TIME
    Prefix<> p3 = Prefix<>("211.99.0.0", "255.255.0.0");

    //4 being the victim doesn't really matter, just need the prefix to be in here
    e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(1, p3));

    //Make an announcement that states 5 and 1 are neighbors. This will create an invalid MAC
    EZAnnouncement attack_announcement3(5, p3.addr, p3.netmask, 299, 5, 2, false, true);
    //The supposed path thus far
    attack_announcement3.as_path.push_back(5);

    e.graph->ases->find(1)->second->process_announcement(attack_announcement3);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();

    /*
     * 1   4
     * | \ |
     * 2   5
     * |   |
     * 3   6
     * 
     * AS 1 should have 2 degrees
     * AS 2 should have 2 degrees
     * AS 5 should have 2 degrees
     * 
     * All others, 1 degree
     */

    //Check if merged
    if(community_detection->identifier_to_component.size() != 1) {
        std::cerr << "Components were not merged correctly!" << std::endl;
        return false;
    }

    //Cannot be sure what identifier survives
    CommunityDetection::Component *component3 = community_detection->identifier_to_component.begin()->second;
    if(component2->hyper_edges.size() != 3) {
        std::cerr << "The hyper edges were not added to the merged component correctly!" << std::endl;
        return false;
    }

    //All should have 1 degree
    for(int i = 4; i <= 6; i++) {
        //Check degree count and existance of AS i
        auto search3 = component3->as_to_degree_count.find(i);
        if(search3 == component3->as_to_degree_count.end()) {
            std::cerr << "AS " << i << " is not in the component after merge!" << std::endl;
            return false;
        }

        if(((i == 1 || i == 2 || i == 5) && search3->second != 2) ||
            ((i != 1 && i != 2 && i != 5) && search3->second != 1)) {
            
            std::cerr << "AS " << i << " has the wrong degree count after merge!" << std::endl;
            return false;
        }
    }

    //All of the ASes with degree count of 1
    auto degree_set_search_merged_1 = component3->degree_sets.find(1);
    if(degree_set_search_merged_1 == component3->degree_sets.end()) {
        std::cerr << "Degree Set 1 was not created after merge!" << std::endl;
        return false;
    }

    std::set<uint32_t> degree_set_merged_1 = degree_set_search_merged_1->second;
    if(degree_set_merged_1.size() != 3) {
        std::cerr << "Degree Set 1 does not have the right ASes after merge!" << std::endl;
        return false;
    }

    if(degree_set_merged_1.find(4) == degree_set_merged_1.end() || degree_set_merged_1.find(3) == degree_set_merged_1.end() || 
            degree_set_merged_1.find(6) == degree_set_merged_1.end()) {
        
        std::cerr << "Degree Set 1 has the wrong ASes in it after the merge!" << std::endl;
        return false; 
    }

    //All of the ASes with degree count of 1
    auto degree_set_search_merged_2 = component3->degree_sets.find(2);
    if(degree_set_search_merged_2 == component3->degree_sets.end()) {
        std::cerr << "Degree Set 2 was not created after merge!" << std::endl;
        return false;
    }

    std::set<uint32_t> degree_set_merged_2 = degree_set_search_merged_2->second;
    if(degree_set_merged_2.size() != 3) {
        std::cerr << "Degree Set 2 does not have the right ASes after merge!" << std::endl;
        return false;
    }

    if(degree_set_merged_2.find(5) == degree_set_merged_2.end() || degree_set_merged_2.find(1) == degree_set_merged_2.end() || 
            degree_set_merged_2.find(2) == degree_set_merged_2.end()) {
        
        std::cerr << "Degree Set 2 has the wrong ASes in it after the merge!" << std::endl;
        return false; 
    }

    return true;
}

/** Test if components can be added to the CD graph
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2--3
 *   /|   
 *  4 5--6 
 * 
 *  5 is claiming that 3 is its origin
 *  1 is claiming that 4 is its origin
 * 
 *  2, 4, and 3 are adoptors
 */
bool ezbgpsec_test_mvc() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);
    e.graph->decide_ranks();

    //2 is the first so it will be the first to report
    e.graph->adopters->push_back(2);
    e.graph->adopters->push_back(4);
    e.graph->adopters->push_back(3);
    // e.graph->adopters->push_back(1);

    //Set them to locally know that they are an adopter
    for(uint32_t asn : *e.graph->adopters) {
        e.graph->ases->find(asn)->second->adopter = true;
    }

    // e.graph->adopters->push_back(6);

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");

    //4 being the victim doesn't really matter, just need the prefix to be in here
    e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(4, p));

    //Make an announcement that states 5 and 3 are neighbors. This will create an invalid MAC
    EZAnnouncement attack_announcement(3, p.addr, p.netmask, 299, 3, 2, false, true);

    //The supposed path thus far
    attack_announcement.as_path.push_back(3);

    e.graph->ases->find(5)->second->process_announcement(attack_announcement);

    Prefix<> p2 = Prefix<>("1.1.0.0", "255.255.0.0");

    //Prefixes need not compete for 2's favor, thus they both need to be checked
    e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(6, p2));

    EZAnnouncement attack_announcement2(4, p2.addr, p2.netmask, 299, 4, 2, false, true);
    attack_announcement2.as_path.push_back(4);//Make the origin on the path

    e.graph->ases->find(1)->second->process_announcement(attack_announcement2);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();

    auto compnent_search = e.communityDetection->identifier_to_component.find(2);
    if(compnent_search == e.communityDetection->identifier_to_component.end()) {
        std::cerr << "Unique identifier is not correct! In MVC" << std::endl;
        return false;
    }

    CommunityDetection::Component *component = compnent_search->second;

    //*************************************************************************
    uint32_t mvc = component->minimum_vertex_cover(2);
    if(mvc != 2) {
        std::cerr << "Global Minimum vertex cover on AS 2 is " << mvc << ", rather than 2" << std::endl; 
        return false;
    }

    mvc = component->local_minimum_vertex_cover(2);
    if(mvc != 2) {
        std::cerr << "Local Minimum vertex cover on AS 2 is " << mvc << ", rather than 2" << std::endl; 
        return false;
    }

    //*************************************************************************
    mvc = component->minimum_vertex_cover(5);
    if(mvc != 1) {
        std::cerr << "Minimum vertex cover on AS 5 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    mvc = component->local_minimum_vertex_cover(5);
    if(mvc != 1) {
        std::cerr << "Local Minimum vertex cover on AS 5 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    //*************************************************************************
    mvc = component->minimum_vertex_cover(1);
    if(mvc != 1) {
        std::cerr << "Minimum vertex cover on AS 1 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    mvc = component->local_minimum_vertex_cover(1);
    if(mvc != 1) {
        std::cerr << "Local Minimum vertex cover on AS 1 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    //*************************************************************************
    mvc = component->minimum_vertex_cover(3);
    if(mvc != 1) {
        std::cerr << "Minimum vertex cover on AS 3 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    mvc = component->local_minimum_vertex_cover(3);
    if(mvc != 1) {
        std::cerr << "Local Minimum vertex cover on AS 3 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    //*************************************************************************
    mvc = component->minimum_vertex_cover(4);
    if(mvc != 1) {
        std::cerr << "Minimum vertex cover on AS 4 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    mvc = component->local_minimum_vertex_cover(4);
    if(mvc != 1) {
        std::cerr << "Local Minimum vertex cover on AS 4 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    return true;
}

/** Test if the local mvc differs from the global mvc
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *     1        5 
 *    / \
 *   2   3
 *    \ /    
 *     4
 * 
 * Yes, 5 has no neighbors. This just makes the test easier. 1 will see the invalid MAC between 4 and 5.
 * 
 * Here, 4 is the attacker claiming to have 5 as the origin for the prefix
 * 2 and 3 are non adopting
 * 5 and 1 are adopting
 * 
 * The local mvc for 1 will be 2
 * The global mvc for 1 will be 1
 */
bool ezbgpsec_test_local_mvc() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);

    e.graph->add_relationship(3, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 3, AS_REL_CUSTOMER);

    e.graph->add_relationship(4, 3, AS_REL_PROVIDER);
    e.graph->add_relationship(3, 4, AS_REL_CUSTOMER);

    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);

    e.graph->ases->insert(std::pair<uint32_t, EZAS*>(5, new EZAS(5)));

    e.graph->decide_ranks();

    //2 is the first so it will be the first to report
    e.graph->adopters->push_back(5);
    e.graph->adopters->push_back(1);
    // e.graph->adopters->push_back(1);

    //Set them to locally know that they are an adopter
    for(uint32_t asn : *e.graph->adopters) {
        e.graph->ases->find(asn)->second->adopter = true;
    }

    // e.graph->adopters->push_back(6);

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");

    //4 being the victim doesn't really matter, just need the prefix to be in here
    e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(4, p));

    //Make an announcement that states 4 and 5 are neighbors. This will create an invalid MAC
    EZAnnouncement attack_announcement(5, p.addr, p.netmask, 299, 5, 2, false, true);

    //The supposed path thus far
    attack_announcement.as_path.push_back(5);

    e.graph->ases->find(4)->second->process_announcement(attack_announcement);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();

    if(e.communityDetection->identifier_to_component.size() != 1) {
        std::cerr << "Local MVC test has " << e.communityDetection->identifier_to_component.size() << " component(s) rather than 1 component" << std::endl;
        return false;
    }

    CommunityDetection::Component *component = (e.communityDetection->identifier_to_component.begin()++)->second;

    if(component->hyper_edges.size() != 2) {
        std::cerr << "Local MVC test has a component with " << component->hyper_edges.size() << " hyper edge(s) rather than 2" << std::endl;
        return false;
    }

    //*************************************************************************
    uint32_t mvc = component->minimum_vertex_cover(1);
    if(mvc != 1) {
        std::cerr << "In the local mvc test, Global Minimum vertex cover on AS 1 is " << mvc << ", rather than 1" << std::endl; 
        return false;
    }

    mvc = component->local_minimum_vertex_cover(1);
    if(mvc != 2) {
        std::cerr << "In the local mvc test, Local Minimum vertex cover on AS 1 is " << mvc << ", rather than 2" << std::endl; 
        return false;
    }

    return true;
}

bool ezbgpsec_test_threshold_filtering() {
    // EZExtrapolator e = EZExtrapolator();
    // e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    // e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    // e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    // e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    // e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    // e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    // e.graph->add_relationship(2, 3, AS_REL_PEER);
    // e.graph->add_relationship(3, 2, AS_REL_PEER);
    // e.graph->add_relationship(5, 6, AS_REL_PEER);
    // e.graph->add_relationship(6, 5, AS_REL_PEER);
    // e.graph->decide_ranks();

    // e.communityDetection->threshold = 2;

    // //2 is the first so it will be the first to report
    // e.graph->adopters->push_back(2);
    // e.graph->adopters->push_back(4);
    // e.graph->adopters->push_back(3);
    // // e.graph->adopters->push_back(1);

    // //Set them to locally know that they are an adopter
    // for(uint32_t asn : *e.graph->adopters) {
    //     e.graph->ases->find(asn)->second->adopter = true;
    // }

    // // e.graph->adopters->push_back(6);

    // Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");

    // //4 being the victim doesn't really matter, just need the prefix to be in here
    // e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(4, p));

    // //Make an announcement that states 5 and 3 are neighbors. This will create an invalid MAC
    // EZAnnouncement attack_announcement(3, p.addr, p.netmask, 299, 3, 2, false, true);

    // //The supposed path thus far
    // attack_announcement.as_path.push_back(3);

    // e.graph->ases->find(5)->second->process_announcement(attack_announcement);

    // Prefix<> p2 = Prefix<>("1.1.0.0", "255.255.0.0");

    // //Prefixes need not compete for 2's favor, thus they both need to be checked
    // e.graph->victim_to_prefixes->insert(std::pair<uint32_t, Prefix<>>(6, p2));

    // EZAnnouncement attack_announcement2(4, p2.addr, p2.netmask, 299, 4, 2, false, true);
    // attack_announcement2.as_path.push_back(4);//Make the origin on the path

    // e.graph->ases->find(1)->second->process_announcement(attack_announcement2);

    // e.propagate_up();
    // e.propagate_down();

    // e.gather_community_detection_reports();

    // auto compnent_search = e.communityDetection->identifier_to_component.find(2);
    // if(compnent_search == e.communityDetection->identifier_to_component.end()) {
    //     std::cerr << "Unique identifier is not correct! In threshold filtering" << std::endl;
    //     return false;
    // }

    // CommunityDetection::Component *component = compnent_search->second;
    // //Nothing should change here since there is no mvc that is greater than the threshold
    // e.communityDetection->threshold_filtering(e.graph);

    // //TODO: replace all of this with degree checks rather than mvc
    // uint32_t mvc = component->minimum_vertex_cover(2);
    // if(mvc != 2) {
    //     std::cerr << "Minimum vertex cover on AS 2 is " << mvc << ", rather than 2" << std::endl; 
    //     return false;
    // }

    // mvc = component->minimum_vertex_cover(5);
    // if(mvc != 1) {
    //     std::cerr << "Minimum vertex cover on AS 5 is " << mvc << ", rather than 1" << std::endl; 
    //     return false;
    // }

    // mvc = component->minimum_vertex_cover(1);
    // if(mvc != 1) {
    //     std::cerr << "Minimum vertex cover on AS 1 is " << mvc << ", rather than 1" << std::endl; 
    //     return false;
    // }

    // mvc = component->minimum_vertex_cover(3);
    // if(mvc != 1) {
    //     std::cerr << "Minimum vertex cover on AS 3 is " << mvc << ", rather than 1" << std::endl; 
    //     return false;
    // }

    // mvc = component->minimum_vertex_cover(4);
    // if(mvc != 1) {
    //     std::cerr << "Minimum vertex cover on AS 4 is " << mvc << ", rather than 1" << std::endl; 
    //     return false;
    // }

    return true;
}