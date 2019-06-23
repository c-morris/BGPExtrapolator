#ifndef RUN_TESTS
#include <iostream>
#include <cstdint>
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
        ("invert-results,i", po::value<bool>()->default_value(true),
          "record ASNs which do *not* have a route to a prefix-origin (smaller results size)")
        ("ram", po::value<bool>()->default_value(false),
          "use the RAM tablespace (it must exist in the db)")
        ("results-table,r", po::value<string>()->default_value(RESULTS_TABLE),
          "name of the results table")
        ("inverse-results-table,o",
          po::value<string>()->default_value(INVERSE_RESULTS_TABLE),
          "name of the inverse results table")
        ("announcements-table,a",
          po::value<string>()->default_value(ANNOUNCEMENTS_TABLE),
          "name of announcements table")
        ("attacker_asn", po::value<std::uint32_t>(),
          "attacker ASN")
        ("victim_asn", po::value<std::uint32_t>(),
          "victim ASN")
        ("victim_prefix", po::value<string>(),
          "victim prefix (i.e. prefix attacker will announce)")
        ("rovpp_version", po::value<int>()->default_value(0),
          "The ROVpp version specifies what capabilites/mechnisms are active. Version 0: No ROVpp, Version 1: Only Negative Announcement (i.e. blackholes), Version 2: Blackholes and Friends, Version 3; Blackholes, Friends, and Preference.");
        //("batch-size", po::value<int>(&batch_size)->default_value(100),
        // "number of prefixes to be used in one propagation cycle")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")){
        cout << desc << endl;
        exit(0);
    }

    // Initialize simulation variables
    std::uint32_t attacker_asn = vm["attacker_asn"].as<std::uint32_t>();
    std::uint32_t victim_asn = vm["victim_asn"].as<std::uint32_t>();
    std::string victim_prefix = vm["victim_prefix"].as<std::string>();
    int rovpp_version = vm["rovpp_version"].as<int>();

    Extrapolator *extrap = new Extrapolator(
        attacker_asn, victim_asn, victim_prefix,
        vm["invert-results"].as<bool>(),
        (vm.count("announcements-table") ? vm["announcements-table"].as<string>() : ANNOUNCEMENTS_TABLE),
        (vm.count("results-table") ? vm["results-table"].as<string>() : RESULTS_TABLE),
        (vm.count("inverse-results-table") ? vm["inverse-results-table"].as<string>() : INVERSE_RESULTS_TABLE),
        (vm.count("ram") ? vm["ram"].as<bool>() : false));
    // TODO make iteration size an option, make the second arg something more reasonable
    extrap->perform_propagation(true, 500000, 100000000000);
    //extrap->perform_propagation(true, 20000, 120001);
    delete extrap;

    return 0;
}
#endif // RUN_TESTS
