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
    e.graph->adopters->push_back(1);

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

    return true;
}