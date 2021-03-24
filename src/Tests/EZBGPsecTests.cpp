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

    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(2)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(4)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(5)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(6)->second->policy = EZAS_TYPE_BGP;

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

    e.graph->ases->find(2)->second->policy = EZAS_TYPE_COMMUNITY_DETECTION_LOCAL;
    e.graph->ases->find(4)->second->policy = EZAS_TYPE_COMMUNITY_DETECTION_LOCAL;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_COMMUNITY_DETECTION_LOCAL;

    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(5)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(6)->second->policy = EZAS_TYPE_BGP;

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");

    //Make an announcement that states 5 and 3 are neighbors. This will create an invalid MAC
    EZAnnouncement attack_announcement(3, p.addr, p.netmask, 299, 3, 2, false, true);
    //The supposed path thus far
    attack_announcement.as_path.push_back(3);

    e.graph->ases->find(5)->second->process_announcement(attack_announcement);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();
    e.communityDetection->process_reports(e.graph);

    std::vector<uint32_t> test_vector = { 2, 5, 3 };
    std::vector<uint32_t> test_vectorb = { 4, 2, 5, 3 };

    if(e.communityDetection->hyper_edges.size() != 2) {
        std::cerr << "The hyper edge was not added to the component properly!" << std::endl;
        std::cerr << "hyper_edges contains " << std::endl;
        for (auto path : e.communityDetection->hyper_edges) {
            for (auto asn : path) {
                std::cerr << asn << ' ';
            }
            std::cerr << std::endl;
        }
        return false;
    }

    if(e.communityDetection->hyper_edges[0] != test_vector || e.communityDetection->hyper_edges[1] != test_vectorb) {
        std::cerr << "The hyper edge path(s) are not correct! Paths are currently:\n";
        for (auto path : e.communityDetection->hyper_edges) {
            for (auto asn : path) {
                std::cerr << asn << ' ';
            }
            std::cerr << std::endl;
        }

        std::cerr << std::endl;
        return false;
    }

    /******************************************************/

    Prefix<> p2 = Prefix<>("1.1.0.0", "255.255.0.0");

    EZAnnouncement attack_announcement2(3, p2.addr, p2.netmask, 299, 3, 2, false, true);
    attack_announcement2.as_path.push_back(3);//Make the origin on the path

    e.graph->ases->find(1)->second->process_announcement(attack_announcement2);

    e.propagate_up();
    e.propagate_down();

    e.gather_community_detection_reports();
    e.communityDetection->process_reports(e.graph);

    if(e.communityDetection->hyper_edges.size() != 4) {
        std::cerr << "The hyper edge was not added to the component properly after the second announcement!" << std::endl;
        std::cerr << "Paths are currently:\n";
        for (auto path : e.communityDetection->hyper_edges) {
            for (auto asn : path) {
                std::cerr << asn << ' ';
            }
            std::cerr << std::endl;
        }

        return false;
    }

    std::vector<uint32_t> test_vector2 = { 2, 1, 3 };
    if(e.communityDetection->hyper_edges[2] != test_vector2) {
        std::cerr << "The hyper edge (index 2) path is not correct after second announcement! Path is currently: { ";

        for(uint32_t asn : e.communityDetection->hyper_edges[2]) {
            std::cerr << asn << ", ";
        }

        std::cerr << "}" << std::endl;

        return false;
    }

    if(e.communityDetection->hyper_edges[0] != test_vector) {
        std::cerr << "The hyper edge (index 0) path is not correct after second announcement! Path is currently: { ";

        for(uint32_t asn : e.communityDetection->hyper_edges[0]) {
            std::cerr << asn << ", ";
        }

        std::cerr << std::endl;

        return false;
    }

    if(e.communityDetection->hyper_edges[0] == e.communityDetection->hyper_edges[1]) {
        std::cerr << "The two hyper edges should not be equal! How did you manage this???" << std::endl;

        return false;
    }

    return true;
}

bool ezbgpsec_test_threshold_filtering_approx() {
    std::vector<uint32_t> edge1 = {4, 1}; 
    std::vector<uint32_t> edge2 = {5, 1};
    std::vector<uint32_t> edge3 = {6, 1};
    std::vector<uint32_t> edge4 = {7, 1};
    std::vector<uint32_t> edge5 = {1};

    CommunityDetection cd(NULL, 2);
    cd.add_hyper_edge(edge1);
    cd.add_hyper_edge(edge2);
    cd.add_hyper_edge(edge3);
    cd.add_hyper_edge(edge4);

    cd.local_threshold_approx_filtering();
    if (cd.blacklist_asns.find(1) == cd.blacklist_asns.end()) {
        return false;
    }

    edge1 = {4, 2, 1}; 
    edge2 = {5, 2, 1};
    edge3 = {6, 2, 1};
    edge4 = {7, 2, 1};

    CommunityDetection cd2(NULL, 2);
    cd2.add_hyper_edge(edge1);
    cd2.add_hyper_edge(edge2);
    cd2.add_hyper_edge(edge3);
    cd2.add_hyper_edge(edge4);

    cd2.local_threshold_approx_filtering();
    if (cd2.blacklist_paths.find(std::vector<uint32_t>({2, 1})) == cd2.blacklist_paths.end()) {
        return false;
    }

    edge1 = {7, 666, 2, 1}; 
    edge2 = {8, 666, 3, 1};
    edge3 = {9, 666, 4, 1};
    edge4 = {6, 666, 5, 1};

    CommunityDetection cd3(NULL, 2);
    cd3.add_hyper_edge(edge1);
    cd3.add_hyper_edge(edge2);
    cd3.add_hyper_edge(edge3);
    cd3.add_hyper_edge(edge4);

    cd3.local_threshold_approx_filtering();
    // Check first level of collapsing
    if (cd3.blacklist_paths.find(std::vector<uint32_t>({666, 2, 1})) == cd3.blacklist_paths.end()) {
        return false;
    }
    if (cd3.blacklist_paths.find(std::vector<uint32_t>({666, 5, 1})) == cd3.blacklist_paths.end()) {
        return false;
    }
    if (cd3.blacklist_paths.find(std::vector<uint32_t>({7, 666, 1})) == cd3.blacklist_paths.end()) {
        return false;
    }
    // Check second level of collapsing
    if (cd3.blacklist_paths.find(std::vector<uint32_t>({666, 1})) == cd3.blacklist_paths.end()) {
        return false;
    }
    // No ASNs should be blacklisted here
    if (cd3.blacklist_asns.size() > 0) {
        return false;
    }
    
    edge1 = {20, 666, 11, 2, 1}; 
    edge2 = {21, 666, 11, 3, 1};
    edge3 = {22, 666, 12, 4, 1};
    edge4 = {23, 666, 12, 5, 1};
    edge5 = {24, 666, 13, 6, 1};

    CommunityDetection cd4(NULL, 2);
    cd4.add_hyper_edge(edge1);
    cd4.add_hyper_edge(edge2);
    cd4.add_hyper_edge(edge3);
    cd4.add_hyper_edge(edge4);
    cd4.add_hyper_edge(edge5);

    cd4.local_threshold_approx_filtering();

    // Check second level of collapsing
    if (cd4.blacklist_paths.find(std::vector<uint32_t>({666, 1})) == cd4.blacklist_paths.end()) {
        return false;
    }
    // No ASNs should be blacklisted here
    if (cd4.blacklist_asns.size() > 0) {
        return false;
    }
 
    edge1 = {20, 666, 11, 2, 1}; 
    edge2 = {21, 666, 11, 3, 1};
    edge3 = {22, 666, 12, 4, 1};
    edge4 = {23, 666, 12, 5, 1};

    CommunityDetection cd5(NULL, 2);
    cd5.add_hyper_edge(edge1);
    cd5.add_hyper_edge(edge2);
    cd5.add_hyper_edge(edge3);
    cd5.add_hyper_edge(edge4);

    cd5.local_threshold_approx_filtering();

    // Check second level of collapsing (should not have found anything here)
    if (cd5.blacklist_paths.find(std::vector<uint32_t>({666, 1})) != cd5.blacklist_paths.end()) {
        return false;
    }
    // No ASNs should be blacklisted here
    if (cd5.blacklist_asns.size() > 0) {
        return false;
    }
    
    edge1 = {30, 20, 666, 11, 1}; 
    edge2 = {30, 21, 666, 11, 1};
    edge3 = {30, 22, 666, 12, 1};
    edge4 = {30, 23, 666, 12, 1};

    CommunityDetection cd6(NULL, 2);
    cd6.add_hyper_edge(edge1);
    cd6.add_hyper_edge(edge2);
    cd6.add_hyper_edge(edge3);
    cd6.add_hyper_edge(edge4);

    cd6.local_threshold_approx_filtering();

    // Check second level of collapsing (should not have found anything here)
    if (cd6.blacklist_paths.find(std::vector<uint32_t>({666, 1})) == cd5.blacklist_paths.end()) {
        return false;
    }
    // Make sure the incorrectly collapsed edge is not here
    if (cd6.blacklist_paths.find(std::vector<uint32_t>({30, 1})) != cd5.blacklist_paths.end()) {
        // This is an expected failure, rather than fixing this function we are
        // taking a different approach that should have no false positives
        return false;
    }
    // No ASNs should be blacklisted here
    if (cd6.blacklist_asns.size() > 0) {
        return false;
    }
    
    // Uncomment for debug
    //std::cerr << "blacklisted paths\n";
    //for (auto path : cd6.blacklist_paths) {
    //    for (auto asn : path) {
    //        std::cerr << asn << ' ';
    //    }
    //    std::cerr << std::endl;
    //}
    //std::cerr << "blacklisted asns\n";
    //for (auto asn : cd6.blacklist_asns) {
    //    std::cerr << asn << ' ';
    //}
    //std::cerr << std::endl;



    return true;
}

bool ezbgpsec_test_is_cover() {
    std::vector<uint32_t> edge1 = {30, 20, 666, 11, 1}; 
    std::vector<uint32_t> edge2 = {30, 21, 666, 11, 1};
    std::vector<uint32_t> edge3 = {30, 22, 666, 12, 1};
    std::vector<uint32_t> edge4 = {30, 23, 666, 12, 1};
    CommunityDetection cd(NULL, 2);
    cd.add_hyper_edge(edge1);
    cd.add_hyper_edge(edge2);
    cd.add_hyper_edge(edge3);
    cd.add_hyper_edge(edge4);

    std::set<uint32_t> s({30, 20, 21, 22, 23, 666, 11, 12, 1});
    std::set<uint32_t> sprime1({30});
    if (!cd.is_cover(sprime1, s)) {
        std::cerr << "is_cover returned false when it should have been true\n";
        return false;
    }
    std::set<uint32_t> sprime2({20});
    if (cd.is_cover(sprime2, s)) {
        std::cerr << "is_cover returned true when it should have been false\n";
        return false;
    }
    return true;
}

bool ezbgpsec_test_gen_ind_asn() {
    std::vector<uint32_t> edge1 = {30, 20, 666, 11, 1}; 
    std::vector<uint32_t> edge2 = {30, 21, 666, 11, 1};
    std::vector<uint32_t> edge3 = {30, 22, 666, 12, 1};
    std::vector<uint32_t> edge4 = {30, 23, 666, 12, 1};
    CommunityDetection cd(NULL, 2);
    cd.add_hyper_edge(edge1);
    cd.add_hyper_edge(edge2);
    cd.add_hyper_edge(edge3);
    cd.add_hyper_edge(edge4);

    std::set<uint32_t> s({30, 20, 21, 22, 23, 666, 11, 12, 1});
    auto ind_asn = cd.gen_ind_asn(s, cd.hyper_edges);

    if (ind_asn[1]   == std::set<uint32_t>({1, 30, 666 })
     && ind_asn[11]  == std::set<uint32_t>({1, 11, 30, 666 })
     && ind_asn[12]  == std::set<uint32_t>({1, 12, 30, 666 })
     && ind_asn[20]  == std::set<uint32_t>({1, 11, 20, 30, 666 })
     && ind_asn[21]  == std::set<uint32_t>({1, 11, 21, 30, 666 })
     && ind_asn[22]  == std::set<uint32_t>({1, 12, 22, 30, 666 })
     && ind_asn[23]  == std::set<uint32_t>({1, 12, 23, 30, 666 })
     && ind_asn[30]  == std::set<uint32_t>({1, 30, 666 })
     && ind_asn[666] == std::set<uint32_t>({1, 30, 666 })) {
        return true;
    } else {
        std::cerr << "ind_asn was not correct, output was\n";
        for (auto pr : ind_asn) {
            std::cerr << pr.first << ": ";
            for (auto a : pr.second) {
                std::cerr << a << ' ';
            }
            std::cerr << '\n';
        }
        return false;
    }
}


bool ezbgpsec_test_get_unique_asns() {
    std::vector<uint32_t> edge1 = {30, 20, 666, 11, 1}; 
    std::vector<uint32_t> edge2 = {30, 21, 666, 11, 1};
    std::vector<uint32_t> edge3 = {30, 22, 666, 12, 1};
    std::vector<uint32_t> edge4 = {30, 23, 666, 12, 1};
    CommunityDetection cd(NULL, 2);
    cd.add_hyper_edge(edge1);
    cd.add_hyper_edge(edge2);
    cd.add_hyper_edge(edge3);
    cd.add_hyper_edge(edge4);

    auto test_s = cd.get_unique_asns(cd.hyper_edges);

    std::set<uint32_t> expected_s({30, 20, 21, 22, 23, 666, 11, 12, 1});
    if (test_s != expected_s) {
        return false;
    }
    return true;
}

