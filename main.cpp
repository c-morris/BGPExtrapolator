#include <iostream>
#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Extrapolator.h"
#include "Tests.h"

int main(int argc, char *argv[]) {
    // test code
    using namespace std;

    
    AS *testas = new AS;
    testas->asn = 42;
    cout << "testas" << endl;
    cout << *testas;
    ASGraph *testgraph = new ASGraph;
    //testgraph->ases->insert(std::pair<uint32_t, AS*>(testas->asn, testas));
    //testgraph->ases->insert(std::pair<uint32_t, AS*>(2, testas));
    testgraph->add_relationship(24, 42, AS_REL_PEER);
    testgraph->add_relationship(24, 48, AS_REL_PROVIDER);
    testgraph->add_relationship(24, 46, AS_REL_CUSTOMER);
    // timing test, took 2 seconds on my laptop
    //for (uint32_t i = 0; i < 80000; i++) {
    //    for (uint32_t j = 0; j < 100; j++) {
    //        testgraph->add_relationship(i, j, AS_REL_PEER);
    //    }
    //    if (i % 10000 == 0) { cerr << "*"; }
    //}
    cout << "testgraph" << endl;
    //cout << *testgraph << endl;
    //cout << *testgraph->ases->find(2)->second << endl;
    delete testas;
    vector<Announcement*> *testanns = new vector<Announcement*>;
    testanns->push_back(new Announcement(13030, 0x01000000, 0xFF000000, 42));
    testanns->push_back(new Announcement(14040, 0x01040000, 0xFFFF0000, 42));
    testanns->push_back(new Announcement(15050, 0x01050000, 0xFFFF0000, 42));
    testanns->push_back(new Announcement(16060, 0x01060000, 0xFFFF0000, 42));
    for (size_t i = 0; i < testanns->size(); i++) {
        cout << *(testanns->at(i)) << endl;
    }
    cout << "testing if 0 is less than 1" << endl;
    cout << (*(testanns->at(0)) < *(testanns->at(1))) << endl;

    cout << "testing if 1 is less than 1" << endl;
    cout << (*(testanns->at(1)) < *(testanns->at(1))) << endl;

    cout << "testing if 1 is equal to 1" << endl;
    cout << (*(testanns->at(1)) == *(testanns->at(1))) << endl;

    cout << "testing if 0 is equal to 1" << endl;
    cout << (*(testanns->at(0)) == *(testanns->at(1))) << endl;

    cout << "testing if 0 is greater than 1" << endl;
    cout << (*(testanns->at(0)) > *(testanns->at(1))) << endl;

    for (size_t i = 0; i < testanns->size(); i++) {
        delete testanns->at(i);
    }
    delete testanns;
    delete testgraph;    
   
    //Start calling Test functions here 
    as_relationship_test();
    as_receive_test();
    as_process_test();
    tarjan_accuracy_test();
    tarjan_size_test();
    cout << "All tests run successfully." << endl;
    return 0;
}
