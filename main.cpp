#include <iostream>
#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Extrapolator.h"
#include "Tests.h"

int main(int argc, char *argv[]) {
    using namespace std;
       
    #ifdef RUN_TESTS
    //as_relationship_test();
    //as_receive_test();
    //as_process_test();
    //tarjan_accuracy_test();
    //tarjan_size_test();
    //send_all_test();
    //tarjan_on_real_data();
    //combine_components_test();
    //decide_ranks_test(); 
    //fully_create_graph_test();
    //give_ann_to_as_path_test();
    //propagate_up_test();
    //propagate_down_test();
    //test_db_connection();
    //select_all_test();
    //test_db_connection();
    //full_propagation_test_a();
    //announcement_comparison_test();
    full_propagation_test_b();
    //query_array_test();
    //distinct_prefixes_test();
    //map_test();
//    SQL_insertion_test();
    cout << "All tests run successfully." << endl;
    #endif

    // put actual main code here
    return 0;
}
