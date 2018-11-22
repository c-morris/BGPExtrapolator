#include <iostream>
#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Extrapolator.h"

int main(int argc, char *argv[]) {
    // test code
    using namespace std;
    AS *testas = new AS;
    testas->asn = 42;
    testas->printDebug();
    ASGraph *testgraph = new ASGraph;
    testgraph->ases->insert(std::pair<uint32_t, AS*>(testas->asn, testas));
    testgraph->ases->insert(std::pair<uint32_t, AS*>(2, testas));
    testgraph->printDebug();
    cout << "testas" << endl;
    cout << *testgraph->ases->find(2)->second << endl;
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

    return 0;
}
