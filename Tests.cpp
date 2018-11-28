#include <assert.h>
#include <random>
#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Extrapolator.h"

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
    return 0;
}

void tarjan_test(){
    std::default_random_engine generator;
    std::uniform_int_distribution<uint32_t> distribution(0,80000);

    ASGraph *testgraph = new ASGraph;
    for (uint32_t i = 0; i < 80000; i++) {
        for (int j = 0; j < 100; j++){
            uint32_t neighbor = distribution(generator);
            testgraph->add_relationship(i,neighbor,AS_REL_PROVIDER);
        }
    }
    return;
}

void set_comparison_test(){
    std::set<uint32_t> set_1;
    std::set<uint32_t> set_2;
   
    for (int i =200; i < 300; i++)
        set_1.insert(set_1.end(),i);
    for (int i =200; i < 300; i++)
        set_2.insert(set_2.end(),i);

    assert(set_1==set_2);
    return 0;
}


