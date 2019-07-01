#ifndef RUN_TESTS
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

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("invert-results,i", 
         po::value<bool>()->default_value(true), 
         "record ASNs which do *not* have a route to a prefix-origin (smaller results size)")
        ("store-depref,d", 
         po::value<bool>()->default_value(false), 
         "store depref results")
        ("iteration-size,s", 
         po::value<uint32_t>()->default_value(50000), 
         "number of prefixes to be used in one iteration cycle")
        ("results-table,r",
         po::value<string>()->default_value(RESULTS_TABLE),
         "name of the results table")
        ("depref-table,p", 
         po::value<string>()->default_value(DEPREF_RESULTS_TABLE),
         "name of the depref table")
        ("inverse-results-table,o",
         po::value<string>()->default_value(INVERSE_RESULTS_TABLE),
         "name of the inverse results table")
        ("announcements-table,a",
         po::value<string>()->default_value(ANNOUNCEMENTS_TABLE),
         "name of announcements table");
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")){
        cout << desc << endl;
        exit(0);
    }

    Extrapolator *extrap = new Extrapolator(vm["invert-results"].as<bool>(),
        vm["store-depref"].as<bool>(),
        (vm.count("announcements-table") ? vm["announcements-table"].as<string>() : ANNOUNCEMENTS_TABLE),
        (vm.count("results-table") ? vm["results-table"].as<string>() : RESULTS_TABLE),
        (vm.count("inverse-results-table") ? vm["inverse-results-table"].as<string>() : INVERSE_RESULTS_TABLE),
        (vm.count("depref-table") ? vm["depref-table"].as<string>() : DEPREF_RESULTS_TABLE),
        (vm["iteration-size"].as<uint32_t>()));
    
    extrap->perform_propagation(true, 500000, 100000000000);
    //extrap->perform_propagation(true, 20000, 120001);
    delete extrap;

    return 0;
}
#endif // RUN_TESTS
