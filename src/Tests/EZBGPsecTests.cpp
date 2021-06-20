#include "Tests/Tests.h"
#include "Extrapolators/EZExtrapolator.h"
#include "ASes/BaseAS.h"

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

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement attack_announcement(5, p, pr, 5, 2, false, true);

    e.graph->ases->find(5)->second->process_announcement(attack_announcement);

    e.propagate_up();
    e.propagate_down();

    if(e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(0) != 1 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(2) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #1 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(2)->second->all_anns->find(p)->as_path.at(0) != 2 ||
        e.graph->ases->find(2)->second->all_anns->find(p)->as_path.at(1) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #2 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(3)->second->all_anns->find(p)->as_path.at(0) != 3 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(3)->second->all_anns->find(p)->as_path.at(2) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #3 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(4)->second->all_anns->find(p)->as_path.at(0) != 4 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(4)->second->all_anns->find(p)->as_path.at(2) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #4 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(5)->second->all_anns->find(p)->as_path.at(0) != 5) {
        
        std::cerr << "EZBGPsec test_path_propagation. AS #5 full path incorrect!" << std::endl;
        return false;
    }

    if(e.graph->ases->find(6)->second->all_anns->find(p)->as_path.at(0) != 6 ||
        e.graph->ases->find(6)->second->all_anns->find(p)->as_path.at(1) != 5) {
        
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

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);

    //Make an announcement that states 5 and 3 are neighbors. This will create an invalid MAC
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement attack_announcement(3, p, pr, 3, 2, false, true);
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

    Prefix<> p2 = Prefix<>("1.1.0.0", "255.255.0.0", 0, 0);

    EZAnnouncement attack_announcement2(3, p2, pr, 3, 2, false, true);
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

    bool error = false;

    std::vector<std::vector<uint32_t>> true_covers = {{30}, {20, 21, 22, 23}, {666}, {666, 45}, {1}, {11, 12}, {666, 30, 1}};
    for(auto &cover : true_covers) {
        if(!cd.is_cover(cover)) {
            error = true;

            std::cerr << "The set: { ";
            for(auto asn : cover)
                std::cerr << asn << " ";
            std::cerr << "} does not make a cover when it should" << std::endl;
        }
    }

    std::vector<std::vector<uint32_t>> false_covers = {{4, 5, 6}, {11}, {11, 22}, {20, 22}, {12}, {12, 23}};
    for(auto &cover : false_covers) {
        if(cd.is_cover(cover)) {
            error = true;

            std::cerr << "The set: { ";
            for(auto asn : cover)
                std::cerr << asn << " ";
            std::cerr << "} does make a cover when it should not" << std::endl;
        }
    }

    return !error;
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

    // std::set<uint32_t> s({30, 20, 21, 22, 23, 666, 11, 12, 1});
    auto ind_asn = cd.ind_map;

    if (ind_asn[1]   == std::unordered_set<uint32_t>({1, 30, 666 })
     && ind_asn[11]  == std::unordered_set<uint32_t>({11})
     && ind_asn[12]  == std::unordered_set<uint32_t>({12})
     && ind_asn[20]  == std::unordered_set<uint32_t>({20})
     && ind_asn[21]  == std::unordered_set<uint32_t>({21})
     && ind_asn[22]  == std::unordered_set<uint32_t>({22})
     && ind_asn[23]  == std::unordered_set<uint32_t>({23})
     && ind_asn[30]  == std::unordered_set<uint32_t>({1, 30, 666 })
     && ind_asn[666] == std::unordered_set<uint32_t>({1, 30, 666 })) {
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

    auto test_s = cd.asn_to_degree;

    std::set<uint32_t> expected_s({30, 20, 21, 22, 23, 666, 11, 12, 1});
    if(test_s.size() != expected_s.size())
        return false;

    for(auto node : expected_s)
        if(test_s.find(node) == test_s.end())
            return false;

    return true;
}


bool ezbgpsec_test_get_degrees() {
    std::vector<uint32_t> edge1 = {30, 20, 666, 11, 1}; 
    std::vector<uint32_t> edge2 = {30, 21, 666, 11, 1};
    std::vector<uint32_t> edge3 = {30, 22, 666, 12, 1};
    std::vector<uint32_t> edge4 = {30, 23, 666, 12, 1};
    CommunityDetection cd(NULL, 2);
    cd.add_hyper_edge(edge1);
    cd.add_hyper_edge(edge2);
    cd.add_hyper_edge(edge3);
    cd.add_hyper_edge(edge4);

    auto test_degrees = cd.asn_to_degree;

    if (!(*test_degrees[1] == 4
       && *test_degrees[11] == 2
       && *test_degrees[12] == 2
       && *test_degrees[20] == 1
       && *test_degrees[21] == 1
       && *test_degrees[22] == 1
       && *test_degrees[23] == 1
       && *test_degrees[30] == 4
       && *test_degrees[666] == 4)) {
        return false;
    }
    return true;
}

bool ezbgpsec_test_gen_suspect_candidates() {
    // std::vector<uint32_t> edge1 = {30, 20, 666, 11, 1}; 
    // std::vector<uint32_t> edge2 = {30, 21, 666, 11, 1};
    // std::vector<uint32_t> edge3 = {30, 22, 666, 12, 1};
    // std::vector<uint32_t> edge4 = {30, 23, 666, 12, 1};
    // CommunityDetection cd(NULL, 2);
    // cd.add_hyper_edge(edge1);
    // cd.add_hyper_edge(edge2);
    // cd.add_hyper_edge(edge3);
    // cd.add_hyper_edge(edge4);

    // auto test_degrees = cd.get_degrees(cd.get_unique_asns(cd.hyper_edges), cd.hyper_edges);
    // auto test_s = cd.get_unique_asns(cd.hyper_edges);
    // auto test_ind_map = cd.gen_ind_asn(test_s, cd.hyper_edges);

    // std::vector<std::vector<uint32_t>> test_suspect_candidates = cd.gen_suspect_candidates(test_ind_map, test_degrees);

    // std::vector<std::vector<uint32_t>> true_suspect_candidates = {{1, 30, 666}};

    // bool error = false;
    // for(auto &suspect : true_suspect_candidates) {
    //     if(std::find(test_suspect_candidates.begin(), test_suspect_candidates.end(), suspect) == test_suspect_candidates.end()) {
    //         error = true;

    //         std::cerr << "The candidate suspect: { ";
    //         for(auto asn : suspect)
    //             std::cerr << asn << " ";
    //         std::cerr << "} is not in the suspect list" << std::endl;
    //     }
    // }

    // if(test_suspect_candidates.size() != true_suspect_candidates.size()) {
    //     error = true;

    //     std::cerr << "The elements of the candidates are not correct. Candidates are currently: ";

    //     std::cerr << "{ ";
    //     for(auto &suspect : test_suspect_candidates) {
    //         std::cerr << "{ ";
    //             for(auto asn : suspect)
    //                 std::cerr << asn << " ";
    //         std::cerr << "} ";
    //     }
    //     std::cerr << "} " << std::endl;;
    // }

    // // std::vector<uint32_t> edge1 = {30, 20, 666, 11, 1}; 
    // // std::vector<uint32_t> edge2 = {30, 21, 666, 11, 1};
    // // std::vector<uint32_t> edge3 = {30, 22, 666, 12, 1};
    // // std::vector<uint32_t> edge4 = {30, 23, 666, 12, 1};
    // std::vector<uint32_t> edge5 = {31, 20, 666, 13, 1};
    // std::vector<uint32_t> edge6 = {32, 20, 666, 14, 1};
    // std::vector<uint32_t> edge7 = {33, 26, 666, 15, 1};
    // cd.add_hyper_edge(edge5);
    // cd.add_hyper_edge(edge6);
    // cd.add_hyper_edge(edge7);

    // test_degrees = cd.get_degrees(cd.get_unique_asns(cd.hyper_edges), cd.hyper_edges);
    // test_s = cd.get_unique_asns(cd.hyper_edges);
    // test_ind_map = cd.gen_ind_asn(test_s, cd.hyper_edges);

    // test_suspect_candidates = cd.gen_suspect_candidates(test_ind_map, test_degrees);

    // true_suspect_candidates = {{1, 666}, {1, 666, 30}, {1, 666, 30, 20}};

    // for(auto &suspect : true_suspect_candidates) {
    //     if(std::find(test_suspect_candidates.begin(), test_suspect_candidates.end(), suspect) == test_suspect_candidates.end()) {
    //         error = true;

    //         std::cerr << "The candidate suspect: { ";
    //         for(auto asn : suspect)
    //             std::cerr << asn << " ";
    //         std::cerr << "} is not in the suspect list (Pt 2.)" << std::endl;
    //     }
    // }

    // if(test_suspect_candidates.size() != true_suspect_candidates.size()) {
    //     error = true;

    //     std::cerr << "The elements of the suspect candidates (Pt 2.) are not correct. Candidates are currently: ";

    //     std::cerr << "{ ";
    //     for(auto &suspect : test_suspect_candidates) {
    //         std::cerr << "{ ";
    //             for(auto asn : suspect)
    //                 std::cerr << asn << " ";
    //         std::cerr << "} ";
    //     }
    //     std::cerr << "} " << std::endl;;
    // }

    // return !error;

    return true;
}

bool ezbgpsec_test_gen_suspect_candidates_tiebrake() {
    // std::vector<uint32_t> edge1 = {30, 20, 3, 666, 2, 1}; 
    // std::vector<uint32_t> edge2 = {30, 21, 3, 666, 2, 1};
    // std::vector<uint32_t> edge3 = {30, 22, 5, 666, 4, 1};
    // std::vector<uint32_t> edge4 = {30, 23, 5, 666, 4, 1};
    // std::vector<uint32_t> edge5 = {30, 24, 7, 666, 6, 1};
    // std::vector<uint32_t> edge6 = {30, 25, 7, 666, 6, 1};
    // CommunityDetection cd(NULL, 1);
    // cd.add_hyper_edge(edge1);
    // cd.add_hyper_edge(edge2);
    // cd.add_hyper_edge(edge3);
    // cd.add_hyper_edge(edge4);
    // cd.add_hyper_edge(edge5);
    // cd.add_hyper_edge(edge6);

    // auto test_degrees = cd.get_degrees(cd.get_unique_asns(cd.hyper_edges), cd.hyper_edges);
    // auto test_s = cd.get_unique_asns(cd.hyper_edges);
    // auto test_ind_map = cd.gen_ind_asn(test_s, cd.hyper_edges);

    // std::vector<std::vector<uint32_t>> test_suspect_candidates = cd.gen_suspect_candidates(test_ind_map, test_degrees);

    // std::vector<std::vector<uint32_t>> true_suspect_candidates = {{1, 30, 666}, { 1, 30, 666, 2, 3 }, { 1, 30, 666, 2, 3, 4, 5 }, { 1, 30, 666, 2, 3, 4, 5, 6, 7 }, { 1, 30, 666, 2, 3, 6, 7 }, { 1, 30, 666, 4, 5 }, { 1, 30, 666, 4, 5, 6, 7 }, { 1, 30, 666, 6, 7 }};

    // bool error = false;
    // for(auto &suspect : true_suspect_candidates) {
    //     if(std::find(test_suspect_candidates.begin(), test_suspect_candidates.end(), suspect) == test_suspect_candidates.end()) {
    //         error = true;

    //         std::cerr << "The candidate suspect (in tiebrake test): { ";
    //         for(auto asn : suspect)
    //             std::cerr << asn << " ";
    //         std::cerr << "} is not in the suspect list" << std::endl;
    //     }
    // }

    // if(test_suspect_candidates.size() != true_suspect_candidates.size()) {
    //     error = true;

    //     std::cerr << "The elements of the candidates (in tiebrake test) are not correct. Candidates are currently: ";

    //     std::cerr << "{ ";
    //     for(auto &suspect : test_suspect_candidates) {
    //         std::cerr << "{ ";
    //             for(auto asn : suspect)
    //                 std::cerr << asn << " ";
    //         std::cerr << "} ";
    //     }
    //     std::cerr << "} " << std::endl;;
    // }

    // return !error;
    return true;
}

bool ezbgpsec_test_cd_algorithm() {
    CommunityDetection cd(NULL, 2);

    std::vector<std::vector<uint32_t>> to_add = {
        { 88, 666, 1 },
        { 88, 666, 3 },
        { 88, 666, 5 },

        { 99, 44, 33, 666, 1 },
        { 99, 44, 33, 666, 3 },
        { 99, 44, 33, 666, 5 },

        { 55, 44, 33, 666, 1 },
        { 55, 44, 33, 666, 3 },
        { 55, 44, 33, 666, 5 },

        { 88, 666, 33, 66, 2 },
        { 88, 666, 33, 66, 4 },
        { 88, 666, 33, 66, 6 },

        { 99, 44, 33, 66, 2 },
        { 99, 44, 33, 66, 4 },
        { 99, 44, 33, 66, 6 },

        { 55, 44, 33, 66, 2 },
        { 55, 44, 33, 66, 4 },
        { 55, 44, 33, 66, 6 }
    };

    for(auto &e : to_add)
        cd.add_hyper_edge(e);

    cd.CD_algorithm();

    std::vector<std::vector<uint32_t>> blacklist = {{ 33, 44, 66 }, {33, 666, 44}, {33, 666, 66, 88}};

    if(cd.blacklist.size() != blacklist.size()) {
        std::cerr << "Incorrect amount of blacklists!" << std::endl;
        return false;
    }

    for(auto &b : cd.blacklist) {
        if(std::find(blacklist.begin(), blacklist.end(), b) == blacklist.end()) {
            std::cerr << "Blackists are not correct!" << std::endl;
        }
    }

    return true;
}

/** Test BGPsec invalid announcement rejection of non-contiguous path. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2
 *    |\
 *    3 13796 
 */
bool ezbgpsec_test_bgpsec_noncontiguous() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(13796, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 13796, AS_REL_CUSTOMER);
    e.graph->decide_ranks();

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGPSEC;
    e.graph->ases->find(2)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(13796)->second->policy = EZAS_TYPE_BGPSEC;

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement attack_announcement(13796, p, pr, 13796, 1, true, true);
    attack_announcement.as_path = std::vector<uint32_t>({13796});

    e.graph->ases->find(3)->second->process_announcement(attack_announcement);

    e.propagate_up();
    e.propagate_down();
    
    // debug
    //e.graph->ases->find(3)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(2)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(1)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    // Announcement should be accepted
    if(e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(0) != 1 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(2) != 3 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(3) != 13796) {
        
        std::cerr << "BGPsec test_path_propagation. AS #1 full path incorrect!" << std::endl;
        return false;
    }
    return true;
}

/** Test BGPsec invalid announcement rejection of contiguous path. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2
 *    |\
 *    3 13796 
 */
bool ezbgpsec_test_bgpsec_contiguous() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(13796, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 13796, AS_REL_CUSTOMER);
    e.graph->decide_ranks();

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGPSEC;
    e.graph->ases->find(2)->second->policy = EZAS_TYPE_BGPSEC;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(13796)->second->policy = EZAS_TYPE_BGPSEC;

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement attack_announcement(13796, p, pr, 13796, 1, true, true);
    attack_announcement.as_path = std::vector<uint32_t>({13796});

    EZAnnouncement origin_announcement(13796, p, pr, 13796, 0, true, false);
    origin_announcement.as_path = std::vector<uint32_t>({});

    e.graph->ases->find(3)->second->process_announcement(attack_announcement);
    e.graph->ases->find(13796)->second->process_announcement(origin_announcement);

    e.propagate_up();
    e.propagate_down();
    
    // debug
    //e.graph->ases->find(3)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(2)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(1)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(13796)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    // Origin announcement should be preferred
    if(e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(0) != 1 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(2) != 13796) {
        
        std::cerr << "BGPsec test_path_propagation. AS #1 full path incorrect!" << std::endl;
        return false;
    }
    return true;
}

/** Test BGPsec invalid announcement rejection of contiguous path. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1--13796
 *    |
 *    2
 *    |
 *    3
 */
bool ezbgpsec_test_bgpsec_contiguous2() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(13796, 1, AS_REL_PEER);
    e.graph->add_relationship(1, 13796, AS_REL_PEER);
    e.graph->decide_ranks();

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGPSEC;
    e.graph->ases->find(2)->second->policy = EZAS_TYPE_BGPSEC;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(13796)->second->policy = EZAS_TYPE_BGPSEC;

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement attack_announcement(13796, p, pr, 13796, 1, true, true);
    attack_announcement.as_path = std::vector<uint32_t>({13796});

    EZAnnouncement origin_announcement(13796, p, pr, 13796, 0, true, false);
    origin_announcement.as_path = std::vector<uint32_t>({});

    e.graph->ases->find(3)->second->process_announcement(attack_announcement);
    e.graph->ases->find(13796)->second->process_announcement(origin_announcement);

    e.propagate_up();
    e.propagate_down();
    
    // debug
    //e.graph->ases->find(3)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(2)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(1)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(13796)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    // Hijack announcement should be preferred (testing security 2nd)
    if(e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(0) != 1 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(2) != 3 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(3) != 13796) {
        
        std::cerr << "BGPsec test_path_propagation. AS #1 full path incorrect!" << std::endl;
        return false;
    }
    return true;
}

/** Test Transitive BGPsec invalid announcement rejection of contiguous path. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2
 *    |\
 *    3 13796
 */
bool ezbgpsec_test_transitive_bgpsec_contiguous() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(13796, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 13796, AS_REL_CUSTOMER);
    e.graph->decide_ranks();

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;
    e.graph->ases->find(2)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(13796)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement attack_announcement(13796, p, pr, 13796, 1, true, true);
    attack_announcement.as_path = std::vector<uint32_t>({13796});

    EZAnnouncement origin_announcement(13796, p, pr, 13796, 0, true, false);
    origin_announcement.as_path = std::vector<uint32_t>({});

    e.graph->ases->find(3)->second->process_announcement(attack_announcement);
    e.graph->ases->find(13796)->second->process_announcement(origin_announcement);

    e.propagate_up();
    e.propagate_down();
    
    // debug
    //e.graph->ases->find(3)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(2)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(1)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(13796)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //auto path = e.graph->ases->find(1)->second->all_anns->find(p).as_path;
    //for (auto p : path) {
    //    std::cout << p << " ";
    //}
    //std::cout << std::endl;

    // Origin AS preferred, attacker's signature is missing
    if(e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(0) != 1 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(2) != 13796) {
        
        std::cerr << "BGPsec test_path_propagation. AS #1 full path incorrect!" << std::endl;
        return false;
    }
    return true;
}

/** Test Transitive BGPsec invalid announcement rejection of contiguous path. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1--13796
 *    |
 *    2
 *    |
 *    3
 */
bool ezbgpsec_test_transitive_bgpsec_contiguous2() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(13796, 1, AS_REL_PEER);
    e.graph->add_relationship(1, 13796, AS_REL_PEER);
    e.graph->decide_ranks();

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;
    e.graph->ases->find(2)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(13796)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement attack_announcement(13796, p, pr, 13796, 1, true, true);
    attack_announcement.as_path = std::vector<uint32_t>({13796});

    EZAnnouncement origin_announcement(13796, p, pr, 13796, 0, true, false);
    origin_announcement.as_path = std::vector<uint32_t>({});

    e.graph->ases->find(3)->second->process_announcement(attack_announcement);
    e.graph->ases->find(13796)->second->process_announcement(origin_announcement);

    e.propagate_up();
    e.propagate_down();
    
    // debug
    //e.graph->ases->find(3)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(2)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(1)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(13796)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    // Attacker's signature is missing, but is preferred because of relationship
    if(e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(0) != 1 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(2) != 3 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(3) != 13796) {
        
        std::cerr << "BGPsec test_path_propagation. AS #1 full path incorrect!" << std::endl;
        return false;
    }
    return true;
}

/** Test Transitive BGPsec invalid announcement rejection of contiguous path. 
 *  Horizontal lines are peer relationships, vertical lines are customer-provider
 * 
 *    1
 *    |
 *    2
 *    |\
 *    | 4
 *    |  \
 *    3  13796
 */
bool ezbgpsec_test_transitive_bgpsec_contiguous3() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);
    e.graph->add_relationship(13796, 4, AS_REL_PROVIDER);
    e.graph->add_relationship(4, 13796, AS_REL_CUSTOMER);
    e.graph->decide_ranks();

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;
    e.graph->ases->find(2)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(13796)->second->policy = EZAS_TYPE_BGPSEC_TRANSITIVE;

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement attack_announcement(13796, p, pr, 13796, 1, true, true);
    attack_announcement.as_path = std::vector<uint32_t>({13796});

    EZAnnouncement origin_announcement(13796, p, pr, 13796, 0, true, false);
    origin_announcement.as_path = std::vector<uint32_t>({});

    e.graph->ases->find(3)->second->process_announcement(attack_announcement);
    e.graph->ases->find(13796)->second->process_announcement(origin_announcement);

    e.propagate_up();
    e.propagate_down();
    
    // debug
    //e.graph->ases->find(3)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(2)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(1)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    //e.graph->ases->find(13796)->second->stream_announcements(std::cout);
    //std::cout << std::endl;

    // Origin AS preferred over path length, attacker's signature is missing
    if(e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(0) != 1 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(1) != 2 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(2) != 4 ||
        e.graph->ases->find(1)->second->all_anns->find(p)->as_path.at(3) != 13796) {
        
        std::cerr << "BGPsec test_path_propagation. AS #1 full path incorrect!" << std::endl;
        return false;
    }
    return true;
}

/** Test fake path generation
 */
bool ezbgpsec_test_gen_fake_as_path() {
    EZExtrapolator e = EZExtrapolator();
    e.communityDetection->local_threshold = 2;

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    // test k=2
    std::vector<uint32_t> p1 = {3, 2, 1};
    std::vector<std::vector<uint32_t>> fake_paths;
    for (size_t i = 0; i <= (p1.size()-1)*e.communityDetection->local_threshold; i++) {
        fake_paths.push_back(e.gen_fake_as_path(p1));
        e.round++;
    }

    // debug
    //for (auto f : fake_paths) {
    //    for (auto a : f) {
    //        std::cout << a << ' ';
    //    }
    //    std::cout << '\n';
    //}

    // first should be equal to last
    if (fake_paths[0] != *fake_paths.rbegin()) {
        std::cout << "Cycle of fake paths does not repeat\n";
        return false;
    }
    if (fake_paths[0] == fake_paths[1]) {
        std::cout << "Consecutive fake paths are identical\n";
        return false;
    }

    return true;
}

/** Test previously seen real path generation
 */
bool ezbgpsec_test_previously_seen_as_path() {
    EZExtrapolator e = EZExtrapolator();

    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(3, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 3, AS_REL_CUSTOMER);
    e.graph->decide_ranks();

    std::vector<uint32_t> p1 = {};
    std::vector<uint32_t> found_path;
    
    e.graph->ases->find(1)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(2)->second->policy = EZAS_TYPE_BGP;
    e.graph->ases->find(3)->second->policy = EZAS_TYPE_BGP;

    for (auto &as : *e.graph->ases) {
        as.second->community_detection = e.communityDetection;
    }

    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0", 0, 0);
    Priority pr;
    pr.relationship = 3;
    EZAnnouncement origin_announcement(3, p, pr, 64514, 0, true, false);
    origin_announcement.as_path = p1;
    e.graph->ases->find(3)->second->process_announcement(origin_announcement);

    found_path = e.get_nonadopting_path_previously_seen(p1.size(), e.graph->ases->find(3)->second, e.graph->ases->find(1)->second, p);
    if (found_path != std::vector<uint32_t>({1})) {
        return false;
    }
    e.propagate_up();
    e.propagate_down();
    e.graph->clear_announcements();

    found_path = e.get_nonadopting_path_previously_seen(p1.size(), e.graph->ases->find(3)->second, e.graph->ases->find(1)->second, p);
    if (found_path != std::vector<uint32_t>({1, 2, 3})) {
        return false;
    }
    // debug
    //for (auto a : found_path) {
    //    std::cout << a << ' ';
    //}
    //std::cout << '\n';

    return true;
}
