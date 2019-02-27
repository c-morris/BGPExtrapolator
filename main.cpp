#include <iostream>
#include <boost/program_options.hpp>

#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Extrapolator.h"
#include "Tests.h"


int main(int argc, char *argv[]) {
    using namespace std;   
    namespace po = boost::program_options;

    #ifdef RUN_TESTS
    //as_relationship_test();
    //as_receive_test();
    //as_process_test();
//    as_process_test_2();
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
//    test_db_connection();
    //full_propagation_test_a();
    //announcement_comparison_test();
//    full_propagation_test_b();
    stub_test();
    //query_array_test();
    //distinct_prefixes_test();
    //map_test();
//    SQL_insertion_test();
//    find_as_path();
    cout << "All tests run successfully." << endl;
    #endif
/*
    int batch_size;
    try{
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("batch-size", po::value<int>(&batch_size)->default_value(100 ),
             "number of prefixes to be used in one propagation cycle")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc,argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")){
            cout << desc << endl;
        }
        if (vm.count("batch-size")){
            cout << "Batch size was ste to "
                << vm["batch-size"].as<int>() << "." << endl;
        }
    }
    catch(exception& e){
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    catch(...){
        cerr << "Unknown Exception!" << endl;
        return 1;
    }
    */
    // put actual main code here
    return 0;
}
