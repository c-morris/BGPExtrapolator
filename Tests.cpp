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

//Test accuracy of ASGraph.tarjan()
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
    //TODO programatically check results
    delete testgraph; 
    return;
}

void tarjan_size_test(){
    std::cout << "Tarjan's Algorithm Size Test..." << std::endl;
    std::default_random_engine generator;
    std::uniform_int_distribution<uint32_t> distribution(0,50000);

    ASGraph *testgraph = new ASGraph;
    for (uint32_t i = 0; i < 50000; i++) {
        for (int j = 0; j < 10; j++){
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

//test AS.process_announcements()
void as_process_test(){
    AS *as = new AS(1);
    std::vector<Announcement> *announcements = new std::vector<Announcement>;
    std::map<Prefix, Announcement> *best_announcements = new std::map<Prefix, Announcement>;

    //Give AS 1 announcements for 4 prefixes each with 4 priorities (16 anns)
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
    //free incoming announcements pointer in object
    delete as->incoming_announcements;
    //give premade announcements and call process_announcements()
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
    delete best_announcements;
    delete as;
}

//Test Extrapolator.send_all_announcements()
void send_all_test(){
    ASGraph *testgraph = new ASGraph;
    testgraph->add_relationship(1,2,AS_REL_PROVIDER);
    testgraph->add_relationship(2,1,AS_REL_CUSTOMER);
    testgraph->add_relationship(1,3,AS_REL_PROVIDER);
    testgraph->add_relationship(3,1,AS_REL_CUSTOMER);
    testgraph->add_relationship(1,4,AS_REL_CUSTOMER);
    testgraph->add_relationship(4,1,AS_REL_PROVIDER);
    testgraph->add_relationship(1,5,AS_REL_CUSTOMER);
    testgraph->add_relationship(5,1,AS_REL_PROVIDER);

    //initialize the announcements held by AS 1
    std::map<Prefix, Announcement> *anns = new std::map<Prefix, Announcement>;
    Announcement ann = Announcement(100, 0x00100000, 0xFF000000,1);
    anns->insert(std::pair<Prefix, Announcement>(ann.prefix,ann));

    testgraph->ases->find(1)->second->all_anns = anns;

    Extrapolator *extrap = new Extrapolator;
    extrap->graph = testgraph;
    //send announcements at AS 1 to it's providers
    extrap->send_all_announcements(1,true);

    //Check that AS 2 got all of the announcements from AS 1
    AS *as_2 = testgraph->ases->find(2)->second;
    for (auto const& ann : *(as_2->incoming_announcements)){
        auto search = anns->find(ann.prefix);
        if(search==anns->end()){ assert(false); }
            assert(search->second == ann);
    }

    //TODO check other ASes
    //
    delete testgraph;
    delete extrap;
    //TODO add counter to make sure no extra announcements were sent  
}

//makes sure database is accessable 
void test_db_connection(){
    SQLQuerier *querier = new SQLQuerier();
    querier->test_connection();
    delete querier;
}

//Creates graph based on relationship data in files
//Could be an argument but is currently hard coded
//Used by some other tests for practical sizes/execution times 
ASGraph* create_graph_from_files(){
    ASGraph *testgraph = new ASGraph;

    std::ifstream file;
    file.open("../peers.csv");
    if (file.is_open()){
        std::string line;
        while(getline(file, line)){
            line.erase(remove(line.begin(),line.end(),' '),line.end());
            if(line.empty() || line[0] == '#' || line[0] == '['){
                continue;
            }
            auto delim_index = line.find(",");
            std::string peer_as_1 = line.substr(0,delim_index);
            std::string peer_as_2 = line.substr(delim_index+1);
            testgraph->add_relationship(std::stoi(peer_as_1), std::stoi(peer_as_2),1);
            testgraph->add_relationship(std::stoi(peer_as_2), std::stoi(peer_as_1),1);
        }
    }
    file.close();
    file.clear();

    file.open("../customer_provider_pairs.csv");
    if (file.is_open()){
        std::string line;
        while(getline(file, line)){
            line.erase(remove(line.begin(),line.end(),' '),line.end());
            if(line.empty() || line[0] == '#' || line[0] == '['){
                continue;
            }
            auto delim_index = line.find(",");
            std::string customer_as = line.substr(0,delim_index);
            std::string provider_as = line.substr(delim_index+1);
            testgraph->add_relationship(std::stoi(customer_as), std::stoi(provider_as),0);
            testgraph->add_relationship(std::stoi(provider_as), std::stoi(customer_as),2);
        }
    }
    file.close();
    file.clear();
    return testgraph;
}

//Test for completion, not for accuracy.
//Prints component size summary to std out
//Writes members of large component (>1000) nodes to file
void tarjan_on_real_data(bool save_large_component){
    ASGraph *testgraph = create_graph_from_files();

    testgraph->tarjan();
    
    //assign integer to each component size to represent number of occurances
    std::map<uint32_t,uint32_t> *component_size_frequencies = new std::map<uint32_t,uint32_t>;
    for (auto const& component : *testgraph->components){
        auto search = component_size_frequencies->find(component->size());
        if (search == component_size_frequencies->end()){
            component_size_frequencies->insert(std::pair<uint32_t,uint32_t>(component->size(),0));
            search = component_size_frequencies->find(component->size());
        }
        search->second++;

        //record members of largest component to file
        if (component->size() > 1000 && save_large_component){
            std::ofstream outfile;
            outfile.open("components.txt");
            outfile << "Component of size " << component->size() << ":\n";
            for (int i = 0; i < component->size(); i++){
                outfile << (component->at(i)) << "\n";
            }
        }
    } 
    
    std::cout << "Component sizes and number of occurances" << std::endl;
    for (auto const& size : *component_size_frequencies){
        std::cout << size.first << ": " << size.second << std::endl;
    }
    delete component_size_frequencies;
    delete testgraph;
}

//combines 
void combine_components_test(){
    //TODO maybe remove dependency on tarjan()
    ASGraph *testgraph = new ASGraph;

    testgraph->add_relationship(1,3,AS_REL_PROVIDER);
    testgraph->add_relationship(2,3,AS_REL_PROVIDER);
    testgraph->add_relationship(3,4,AS_REL_PROVIDER);
    testgraph->add_relationship(3,1,AS_REL_PROVIDER);
    testgraph->add_relationship(4,3,AS_REL_CUSTOMER);
    testgraph->add_relationship(4,5,AS_REL_PROVIDER);
    testgraph->add_relationship(5,4,AS_REL_CUSTOMER);
    testgraph->add_relationship(6,1,AS_REL_PROVIDER);
    testgraph->add_relationship(1,6,AS_REL_CUSTOMER);

    testgraph->tarjan();
    testgraph->combine_components();
    //TODO propgramatically confirm results
    for (auto const& as : *testgraph->ases){
        std::cout << *as.second << std::endl;
    }
    
}

//Decides node ranks with  test relationship data
void decide_ranks_test(){
    ASGraph *testgraph = new ASGraph;
    
    testgraph->add_relationship(1,3,AS_REL_PROVIDER);
    testgraph->add_relationship(2,3,AS_REL_PROVIDER);
    testgraph->add_relationship(3,1,AS_REL_CUSTOMER);
    testgraph->add_relationship(3,2,AS_REL_CUSTOMER);
    testgraph->add_relationship(3,4,AS_REL_PROVIDER);
    testgraph->add_relationship(4,3,AS_REL_CUSTOMER);
    testgraph->add_relationship(4,5,AS_REL_PROVIDER);
    testgraph->add_relationship(5,4,AS_REL_CUSTOMER);
    testgraph->add_relationship(6,1,AS_REL_PROVIDER);
    testgraph->add_relationship(1,6,AS_REL_CUSTOMER);

    testgraph->decide_ranks();
 
    //TODO propgramatically confirm results
    int i = 0;
    while(!testgraph->ases_by_rank->at(i)->empty()){
        std::cout << "Rank " << i << ": ";
        for (const uint32_t asn : *(testgraph->ases_by_rank->at(i))){
            std::cout << asn << ", ";
        }
        std::cout << std::endl;
        i++;
    }
}

//Calls all graph creation functions on external relationship data
void fully_create_graph_test(){
    ASGraph *testgraph = create_graph_from_files();
    testgraph->tarjan();
    testgraph->combine_components();
    testgraph->decide_ranks();
    delete testgraph;
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
    return;
}
