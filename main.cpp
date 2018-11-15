#include <iostream>
#include "AS.h"
#include "ASGraph.h"

int main(int argc, char *argv[]) {
    using namespace std;
    cout << "aaa\n";
    AS *testas = new AS;
    testas->setASN(42);
    testas->printDebug();
    ASGraph *testgraph = new ASGraph;
    testgraph->ases->insert(std::pair<uint32_t, AS*>(testas->asn, testas));
    testgraph->printDebug();
   // destroy testas;
    return 0;
}
