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

/** Test 
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
    Logger::setFolder("./Logs/");

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