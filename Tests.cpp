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
    std::map<Prefix<>, Announcement> *best_announcements = new std::map<Prefix<>, Announcement>;

    //Give AS 1 announcements for 4 prefixes each with 4 priorities (16 anns)
    for(int i = 1; i < 4; i ++){
        Announcement best_ann = Announcement(30*i,0x00100000*i,0xFF000000,i);
        best_ann.priority = 3.0;
        best_announcements->insert(std::pair<Prefix<>,Announcement>(
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

//makes sure database is accessable 
void test_db_connection(){
    //connection is established on initialization
    SQLQuerier *querier = new SQLQuerier();
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
            for (uint32_t i = 0; i < component->size(); i++){
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

//////////////////////////
//  Extrapolator Tests  //
//////////////////////////

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
    std::map<Prefix<>, Announcement> *anns = new std::map<Prefix<>, Announcement>;
    Announcement ann = Announcement(100, 0x00100000, 0xFF000000,1);
    anns->insert(std::pair<Prefix<>, Announcement>(ann.prefix,ann));

    testgraph->ases->find(1)->second->all_anns = anns;

    Extrapolator *extrap = new Extrapolator;
    delete extrap->graph;
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
    delete extrap;
    //TODO add counter to make sure no extra announcements were sent  
}

void give_ann_to_as_path_test(){
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

    testgraph->process();
    Extrapolator *extrap = new Extrapolator;
    extrap->graph = testgraph;
    
    std::vector<uint32_t> *as_path = new std::vector<uint32_t>{1,3,4,5};
    Prefix<> p;
    p.addr = 0x01000000;
    p.netmask = 0xFF000000;
    extrap->give_ann_to_as_path(as_path,p,"hop");
    p.addr = 0x01040000;
    p.netmask = 0xFFFF0000;
    extrap->give_ann_to_as_path(as_path,p,"hop");

    for (auto const& as : *testgraph->ases){
        std::cout << "Anns at " << as.second->asn << ": " << std::endl;
        for (auto &ann : *as.second->all_anns){
            std::cout << "\t" << ann.second << std::endl;
        }
    }
    delete extrap;
}

void propagate_up_test(){
    ASGraph *testgraph = new ASGraph;
    testgraph->add_relationship(1,2,AS_REL_PROVIDER);
    testgraph->add_relationship(1,3,AS_REL_PROVIDER);
    testgraph->add_relationship(1,4,AS_REL_PROVIDER);
    testgraph->add_relationship(4,1,AS_REL_CUSTOMER);
    testgraph->add_relationship(3,1,AS_REL_CUSTOMER);
    testgraph->add_relationship(2,1,AS_REL_CUSTOMER);
    
    testgraph->process();
    Extrapolator *extrap = new Extrapolator;
    extrap->graph = testgraph;
    Announcement *testann = new Announcement(1,0x01000000, 0xFF000000, 3, 1, "hop");
    testgraph->ases->find(1)->second->receive_announcement(*testann);
    
    extrap->propagate_up();

    for (auto const& as : *testgraph->ases){
        std::cout << "Anns at " << as.second->asn << ": " << std::endl;
        for (auto &ann : *as.second->all_anns){
            std::cout << "\t" << ann.second << std::endl;
        }
    }
    delete extrap;
}

void propagate_down_test(){
    ASGraph *testgraph = new ASGraph;
    testgraph->add_relationship(1,2,AS_REL_CUSTOMER);
    testgraph->add_relationship(1,3,AS_REL_CUSTOMER);
    testgraph->add_relationship(1,4,AS_REL_CUSTOMER);
    testgraph->add_relationship(4,1,AS_REL_PROVIDER);
    testgraph->add_relationship(3,1,AS_REL_PROVIDER);
    testgraph->add_relationship(2,1,AS_REL_PROVIDER);
    
    testgraph->process();
    Extrapolator *extrap = new Extrapolator;
    extrap->graph = testgraph;
    Announcement *testann = new Announcement(1,0x01000000, 0xFF000000, 3, 1, "hop");
    testgraph->ases->find(1)->second->receive_announcement(*testann);
    
    extrap->propagate_down();

    for (auto const& as : *testgraph->ases){
        std::cout << "Anns at " << as.second->asn << ": " << std::endl;
        for (auto &ann : *as.second->all_anns){
            std::cout << "\t" << ann.second << std::endl;
        }
    }
    delete extrap;
   
}

void select_all_test(){
    SQLQuerier *querier = new SQLQuerier();
    pqxx::result R = querier->select_from_table("test_customer_providers");
    std::cout <<"Rows in \"test_customer_providers\": "<< R.size() << std::endl;
    delete querier;
}

void full_propagation_test_a(){ 
    using namespace std::chrono;

    Extrapolator *extrap = new Extrapolator;

    //Maybe change this to use C++ ip struct instead of doing conversion here
    //Getting records from db
    pqxx::result R = extrap->querier->select_ann_records("simplified_elements", "",1000);
    auto start = high_resolution_clock::now();
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        Prefix<> p(c[0].as<std::string>(), c[1].as<std::string>());
        std::vector<uint32_t> *as_path = new std::vector<uint32_t>;
        std::string path_as_string = c[2].as<std::string>();
        //TODO add parsing function for paths
        //libpq++ doesn't support arrays and are recommended to be parsed from strings
        //this is written for 'simplified elements' table with paths of length 1
        char brackets[] = "{}";
        for (uint32_t i = 0; i < strlen(brackets); ++i)
            path_as_string.erase(std::remove(path_as_string.begin(),path_as_string.end(), brackets[i]), path_as_string.end());
        as_path->push_back(std::stoi(path_as_string));
//        uint32_t hop = c[3].as<std::uint32_t>();
        //No hop in 'simplified_elements'
        std::string hop = "hop";

        extrap->give_ann_to_as_path(as_path,p,hop);

    }
    auto give_ann_time = high_resolution_clock::now(); 
    extrap->propagate_up();
    auto propagate_up_time = high_resolution_clock::now(); 
    extrap->propagate_down();
    auto propagate_down_time = high_resolution_clock::now(); 

    auto give_duration = duration_cast<seconds>(give_ann_time - start);
    auto up_duration = duration_cast<seconds>(propagate_up_time - give_ann_time);
    auto down_duration = duration_cast<seconds>(propagate_down_time - propagate_up_time);
    
    std::cout << "Give ann time: " << give_duration.count()<< std::endl;
    std::cout << "Propagate up time: " << up_duration.count()<< std::endl;
    std::cout << "Propagate down time: " << down_duration.count()<< std::endl;

    delete extrap;
}

void full_propagation_test_b(){
    Extrapolator *extrap = new Extrapolator;
    extrap->perform_propagation(true, 10, 100);
    return;
}

void distinct_prefixes_test(){
    SQLQuerier *querier = new SQLQuerier;
    pqxx::result R = querier->select_distinct_prefixes_from_table("elements");
//    for (pqxx::result::size_type i = 0; i !=R.size();++i){
    for (pqxx::result::size_type i = 0; i <10;++i){
        std::cout << R[i]["prefix"].c_str() << std::endl;
    }

//    for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c){
//        if(i > 10){break;}
//        std::cout << c[0].as<std::string>() <<std::endl;
//        i++;
//    }

    delete querier;
}

void announcement_comparison_test() {
    std::vector<Announcement*> *testanns = new std::vector<Announcement*>;
    testanns->push_back(new Announcement(13030, 0x01000000, 0xFF000000, 42));
    testanns->push_back(new Announcement(14040, 0x01040000, 0xFFFF0000, 42));
    testanns->push_back(new Announcement(15050, 0x01050000, 0xFFFF0000, 42));
    testanns->push_back(new Announcement(16060, 0x01060000, 0xFFFF0000, 42));
    assert(*(testanns->at(0)) < *(testanns->at(1)));
    assert(!(*(testanns->at(1)) < *(testanns->at(1))));
    assert(*(testanns->at(1)) == *(testanns->at(1)));
    assert(!(*(testanns->at(0)) == *(testanns->at(1))));
    assert(!(*(testanns->at(0)) > *(testanns->at(1))));
    for (size_t i = 0; i < testanns->size(); i++) {
        delete testanns->at(i);
    }
    delete testanns;
}
