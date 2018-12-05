#include "Tests.h"

//Tests the functionality of ASGraph.add_relationship
void as_relationship_test(){
    using namespace std;
    
    ASGraph *testgraph = new ASGraph;
    //Create 1000 ASes and add 100 providers, peers, customers to all of them
    for (uint32_t i = 0; i < 1000; i++) {
        for (uint32_t j = 0; j < 100; j++){            
            testgraph->add_relationship(i,j,AS_REL_PROVIDER);
            testgraph->add_relationship(i,j*2,AS_REL_PEER);
            testgraph->add_relationship(i,j*3,AS_REL_CUSTOMER);
        }
    }
    //Create sets that should reflect the providers, peers, customers
    //assigned to the previously created ASes.
    std::set<uint32_t> providers;
    std::set<uint32_t> peers;
    std::set<uint32_t> customers;
    for (int i =0; i < 100; i++){
        providers.insert(providers.end(),i);
        peers.insert(peers.end(),i*2);
        customers.insert(customers.end(),i*3);
    }
    //Assert that the 1000 ASes have the given providers, peers, customers
    for (auto const& as: *(testgraph->ases)){
        assert(*(as.second->customers)==customers);
        assert(*(as.second->peers)==peers);
        assert(*(as.second->providers)==providers);
    }

    delete testgraph;
    return;
}

//TODO finish this test
void tarjan_accuracy_test(){
    std::cout << "Tarjan's Algorithm Accuracy Test..." << std::endl;
    ASGraph *testgraph = new ASGraph;

    testgraph->add_relationship(1,3,AS_REL_PROVIDER);
    testgraph->add_relationship(2,3,AS_REL_PROVIDER);
    testgraph->add_relationship(3,4,AS_REL_PROVIDER);
    testgraph->add_relationship(3,1,AS_REL_PROVIDER);
    testgraph->add_relationship(4,3,AS_REL_CUSTOMER);
    testgraph->add_relationship(4,5,AS_REL_PROVIDER);
    testgraph->add_relationship(5,4,AS_REL_CUSTOMER);

    testgraph->tarjan();
    
    std::cout << "Strongly Connected Components:" << std::endl;
    for (size_t i = 0; i < testgraph->components->size(); i++){
        for (const auto j: *testgraph->components[0][i])
            std::cout << j << ',';
        std::cout << std::endl;
    }
    delete testgraph; 
    return;
}

void tarjan_size_test(){
    std::cout << "Tarjan's Algorithm Size Test..." << std::endl;
    std::default_random_engine generator;
    std::uniform_int_distribution<uint32_t> distribution(0,50);

    ASGraph *testgraph = new ASGraph;
    for (uint32_t i = 0; i < 50; i++) {
        for (int j = 0; j < 100; j++){
            uint32_t neighbor = distribution(generator);
            testgraph->add_relationship(i,neighbor,AS_REL_PROVIDER);
            testgraph->add_relationship(neighbor,i,AS_REL_CUSTOMER);
        }
    }
    testgraph->tarjan();
    delete testgraph;
    return;
}

//Tests the functionality of AS.receive_announcements()
void as_receive_test(){
    AS as = AS(1);
    std::vector<Announcement> announcements;
    //create 10 announcements with somewhat irrelivent args
    for(int i = 1; i < 11; i ++){
        announcements.push_back(Announcement(30*i,0x00100000*i,0xFF000000,i));
    }
    //give announcements to 'as'
    as.receive_announcements(announcements);
    //assert that announcements held by 'as' are equiv to those given
    assert(*(as.incoming_announcements) == announcements);
}

void as_process_test(){
    AS *as = new AS(1);
    std::vector<Announcement> *announcements = new std::vector<Announcement>;
    std::map<Prefix, Announcement> *best_announcements = new std::map<Prefix, Announcement>;
    for(int i = 1; i < 4; i ++){
        Announcement best_ann = Announcement(30*i,0x00100000*i,0xFF000000,i);
        best_ann.priority = 3.0;
        best_announcements->insert(std::pair<Prefix,Announcement>(
            best_ann.prefix,best_ann));
        for(double j = 0.0; j < 4; j++){
            Announcement ann = Announcement(30*i,0x00100000*i,0xFF000000,i);
            ann.priority = j;
            announcements->push_back(ann);
        }
    }
    delete as->incoming_announcements;
    as->incoming_announcements = announcements;
    as->process_announcements();

    //For equivelency, check that as.all_anns is a subset of best_announcements
    //and best_announcements is a subset of as.all_anns.
    for (auto &ann : *as->all_anns){
        //assert entry with prefix is found and announcement is the same
        auto search = best_announcements->find(ann.second.prefix);
        if(search==best_announcements->end()){ assert(false); }
        assert(search->second == ann.second);
    }
    for (auto &ann : *best_announcements){
        auto search = as->all_anns->find(ann.second.prefix);
        if(search==as->all_anns->end()){ assert(false); }
        assert(search->second == ann.second);
    }
//    delete announcements;
    delete best_announcements;
    delete as;
}

/*
void send_all_test(){
    ASGraph *testgraph = new ASGraph; 
    testgraph->add_relationship(1,2,AS_REL_PROVIDER);
    testgraph->add_relationship(1,3,AS_REL_PROVIDER);
    testgraph->add_relationship(1,4,AS_REL_CUSTOMER);
    testgraph->add_relationship(1,5,AS_REL_CUSTOMER);
    
    anns = new std::map<Prefix, Announcement>;
    prefix = 
    anns->insert(std::pair<Prefix, Announcement>(ann.prefix,ann));
    
    Extrapolator *extrap = new Extrapolator;
    extrap->send_all_announcements(1,true);

    for (auto &ann : *sent_anns){
        auto search = best_announcements->find(ann.second.prefix);
        if(search==best_announcements->end()){ assert(false); }
        assert(search->second == ann.second);
    }
    for (auto &ann : *as.incoming_announcements){
        auto search = as.all_anns->find(ann.second.prefix);
        if(search==as.all_anns->end()){ assert(false); }
        assert(search->second == ann.second);
    }   

}
*/

void test_db_connection(){
    SQLQuerier *querier = new SQLQuerier();
    querier->test_connection();
}

void set_comparison_test(){
    std::set<uint32_t> set_1;
    std::set<uint32_t> set_2;
   
    for (int i =200; i < 300; i++)
        set_1.insert(set_1.end(),i);
    for (int i =200; i < 300; i++)
        set_2.insert(set_2.end(),i);

    assert(set_1==set_2);
    return;
}
