/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#ifndef RUN_TESTS
#include <iostream>
#include <boost/program_options.hpp>
#include <thread>
#include <semaphore.h>

#include "ASes/AS.h"
#include "Graphs/ASGraph.h"
#include "Announcements/Announcement.h"
#include "Extrapolators/Extrapolator.h"
#include "Extrapolators/ROVppExtrapolator.h"
#include "Extrapolators/EZExtrapolator.h"
#include "Tests/Tests.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
// #include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

void intro() {
    // This needs to be finished
    BOOST_LOG_TRIVIAL(info) << "***** BGP Extrapolator v0.3 *****";
    BOOST_LOG_TRIVIAL(info) << "This is free software: you are free to change and redistribute it.";
    BOOST_LOG_TRIVIAL(info) << "There is NO WARRANTY, to the extent permitted by law.";
}

int main(int argc, char *argv[]) {
    using namespace std;   
    // Don't sync iostreams with printf
    ios_base::sync_with_stdio(false);
    namespace po = boost::program_options;
    // Handle parameters
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("rovpp,v", 
         po::value<bool>()->default_value(false), 
         "flag for rovpp run")
        ("ezbgpsec,z", 
         po::value<uint32_t>()->default_value(0), 
         "number of rounds for ezbgpsec run")
        ("num-in-between,n", 
         po::value<uint32_t>()->default_value(DEFAULT_NUM_ASES_BETWEEN_ATTACKER), 
         "number of in between ASes for ezbgpsec run")
        ("random,b", 
         po::value<bool>()->default_value(DEFAULT_RANDOM_TIEBRAKING), 
         "disables random tiebraking for testing")
        ("store-results", 
         po::value<bool>()->default_value(DEFAULT_STORE_RESULTS), 
         "save results allowing reconstruction of local RIBs.")
        ("store-inverse-results,i", 
         po::value<bool>()->default_value(DEFAULT_STORE_INVERT_RESULTS), 
         "save ASNs which do *not* have a route to a prefix-origin")
        ("store-depref,d",
         po::value<bool>()->default_value(DEFAULT_STORE_DEPREF_RESULTS), 
         "record the second-best announcements for each prefix")
        ("iteration-size,s", 
         po::value<uint32_t>()->default_value(DEFAULT_ITERATION_SIZE), 
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
        ("full-path-results-table",
         po::value<string>()->default_value(FULL_PATH_RESULTS_TABLE),
         "name of the full path results table")
        ("announcements-table,a",
         po::value<string>()->default_value(ANNOUNCEMENTS_TABLE),
         "name of announcements table")
        ("simulation-table,f",
         po::value<string>()->default_value(ROVPP_SIMULATION_TABLE),
         "name of simulation announcements table")
        ("tracked-ases-table,u",
         po::value<string>()->default_value(ROVPP_TRACKED_ASES_TABLE),
         "name of tracked ases table for attackers and victims")
        ("policy-tables,t",
         po::value<vector<string>>(),
         "space-separated names of ROVpp policy tables")
        ("prop-twice,k",
         po::value<bool>()->default_value(true),
         "flag whether or not to propagate twice (ROVpp only)")
        ("log-std-out,l",
         po::value<bool>()->default_value(true),
         "enables logging into the console, best used for debugging only")
        ("log-folder,g",
         po::value<string>()->default_value(""),
         "path to a log folder, enables logging into a file, best used for debugging only")
        ("severity-level,c",
         po::value<unsigned int>()->default_value(0),
         "severity of errors to be logged, from 0 (trace) to 5 (fatal)")
        ("config-section", po::value<string>()->default_value("bgp"), "section of the config file")
        ("exclude-monitor", po::value<int>()->default_value(-1),
         "exclude announcements from a particular monitor ASN")
        ("full-path-asns",
         po::value<vector<uint32_t>>(),
         "output these ASNs with their full AS_PATH in a separate table")
        ("mh-propagation-mode", 
         po::value<uint32_t>()->default_value(DEFAULT_MH_MODE),
         "multi-home propagation mode, 0 - off, 1 - propagate from mh to providers in some cases (automatic), 2 - no propagation from mh, 3 - propagation from mh to peers")
         ("origin-only",
         po::value<bool>()->default_value(DEFAULT_ORIGIN_ONLY),
         "seed announcements only at the origin")
         ("ipv6-mode",
         po::value<bool>()->default_value(DEFAULT_IPV6_MODE),
         "enables IPv6 mode");

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
        cout << desc << endl;
        exit(0);
    }

    // Logging
    unsigned int severity_level = vm["severity-level"].as<unsigned int>();
    bool log_std_out = vm["log-std-out"].as<bool>();
    string log_folder = vm["log-folder"].as<string>();
    if ((severity_level < 0) || (severity_level > 5)) {
       Logger::init_logger(log_std_out, log_folder, 0);
    } else {
       Logger::init_logger(log_std_out, log_folder, severity_level);
    }

    // Handle intro information
    intro();

    // Record full path asns
    vector<uint32_t> *full_path_asns = NULL;
    vector<uint32_t> asn_vect;
    if (vm.count("full-path-asns")) {
        asn_vect = vm["full-path-asns"].as<vector<uint32_t>>();
        full_path_asns = &asn_vect;
    }
    
    // Check for ROV++ mode
    if (vm["rovpp"].as<bool>()) {
         ROVppExtrapolator *extrap = new ROVppExtrapolator(
            (vm.count("policy-tables") ?
                vm["policy-tables"].as<vector<string>>() : 
                vector<string>()),
            (vm.count("announcements-table") ? 
                vm["announcements-table"].as<string>() : 
                ROVPP_ANNOUNCEMENTS_TABLE),
            (vm.count("results-table") ?
                vm["results-table"].as<string>() :
                ROVPP_RESULTS_TABLE),
            (vm.count("full-path-results-table") ?
                vm["full-path-results-table"].as<string>() :
                FULL_PATH_RESULTS_TABLE),
            (vm.count("tracked-ases-table") ?
                vm["tracked-ases-table"].as<string>() : 
                ROVPP_TRACKED_ASES_TABLE),
            (vm.count("simulation-table") ?
                vm["simulation-table"].as<string>() : 
                ROVPP_SIMULATION_TABLE),
            vm["config-section"].as<string>(),
            vm["exclude-monitor"].as<int>(),
            vm["origin-only"].as<bool>());
            
        // Run propagation
        bool prop_twice = vm["prop-twice"].as<bool>();
        extrap->perform_propagation(prop_twice);
        // Clean up
        delete extrap;
    } else if(vm["ezbgpsec"].as<uint32_t>()) {
        // Instantiate Extrapolator
        EZExtrapolator *extrap = new EZExtrapolator(
            vm["random"].as<bool>(),
            vm["store-results"].as<bool>(),
            vm["store-inverse-results"].as<bool>(),
            vm["store-depref"].as<bool>(),
            (vm.count("announcements-table") ? 
                vm["announcements-table"].as<string>() : 
                ANNOUNCEMENTS_TABLE),
            (vm.count("results-table") ?
                vm["results-table"].as<string>() :
                RESULTS_TABLE),
            (vm.count("inverse-results-table") ?
                vm["inverse-results-table"].as<string>() : 
                INVERSE_RESULTS_TABLE),
            (vm.count("depref-table") ?
                vm["depref-table"].as<string>() : 
                DEPREF_RESULTS_TABLE),
            (vm.count("full-path-results-table") ?
                vm["full-path-results-table"].as<string>() :
                FULL_PATH_RESULTS_TABLE),
            vm["config-section"].as<string>(),
            vm["iteration-size"].as<uint32_t>(),
            vm["ezbgpsec"].as<uint32_t>(), // number of rounds
            vm["num-in-between"].as<uint32_t>(),
            vm["exclude-monitor"].as<int>(),
            vm["mh-propagation-mode"].as<uint32_t>(),
            vm["origin-only"].as<bool>(),
            full_path_asns);
            
        // Run propagation
        extrap->perform_propagation();

        // Clean up
        delete extrap;
    } else if (vm["ipv6-mode"].as<bool>()) {
        // Instantiate Extrapolator
        Extrapolator<uint128_t> *extrap = new Extrapolator<uint128_t> (vm["random"].as<bool>(),
            vm["store-results"].as<bool>(),
            vm["store-inverse-results"].as<bool>(),
            vm["store-depref"].as<bool>(),
            (vm.count("announcements-table") ? 
                vm["announcements-table"].as<string>() : 
                ANNOUNCEMENTS_TABLE),
            (vm.count("results-table") ?
                vm["results-table"].as<string>() :
                RESULTS_TABLE),
            (vm.count("inverse-results-table") ?
                vm["inverse-results-table"].as<string>() : 
                INVERSE_RESULTS_TABLE),
            (vm.count("depref-table") ?
                vm["depref-table"].as<string>() : 
                DEPREF_RESULTS_TABLE),
            (vm.count("full-path-results-table") ?
                vm["full-path-results-table"].as<string>() :
                FULL_PATH_RESULTS_TABLE),
            vm["config-section"].as<string>(),
            vm["iteration-size"].as<uint32_t>(),
            vm["exclude-monitor"].as<int>(),
            vm["mh-propagation-mode"].as<uint32_t>(),
            vm["origin-only"].as<bool>(),
            full_path_asns);
            
        // Run propagation
        extrap->perform_propagation();
        // Clean up
        delete extrap;
    } else {
        // Instantiate Extrapolator
        Extrapolator<> *extrap = new Extrapolator<> (vm["random"].as<bool>(),
            vm["store-results"].as<bool>(),
            vm["store-inverse-results"].as<bool>(),
            vm["store-depref"].as<bool>(),
            (vm.count("announcements-table") ? 
                vm["announcements-table"].as<string>() : 
                ANNOUNCEMENTS_TABLE),
            (vm.count("results-table") ?
                vm["results-table"].as<string>() :
                RESULTS_TABLE),
            (vm.count("inverse-results-table") ?
                vm["inverse-results-table"].as<string>() : 
                INVERSE_RESULTS_TABLE),
            (vm.count("depref-table") ?
                vm["depref-table"].as<string>() : 
                DEPREF_RESULTS_TABLE),
            (vm.count("full-path-results-table") ?
                vm["full-path-results-table"].as<string>() :
                FULL_PATH_RESULTS_TABLE),
            vm["config-section"].as<string>(),
            vm["iteration-size"].as<uint32_t>(),
            vm["exclude-monitor"].as<int>(),
            vm["mh-propagation-mode"].as<uint32_t>(),
            vm["origin-only"].as<bool>(),
            full_path_asns);
            
        // Run propagation
        extrap->perform_propagation();
        // Clean up
        delete extrap;
    }
    return 0;
}
#endif // RUN_TESTS
