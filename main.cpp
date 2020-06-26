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

#include "Logger.h"
#include "ASes/AS.h"
#include "Graphs/ASGraph.h"
#include "Announcements/Announcement.h"
#include "Extrapolators/Extrapolator.h"
#include "Extrapolators/ROVppExtrapolator.h"
#include "Extrapolators/EZExtrapolator.h"
#include "Tests/Tests.h"

void intro() {
    // This needs to be finished
    std::cout << "***** Routing Extrapolator v0.2 *****" << std::endl;
    std::cout << "Copyright (C) someone, somewhere, probably." << std::endl;
    std::cout << "License... is probably important." << std::endl;
    std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
    std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
}

int main(int argc, char *argv[]) {
    using namespace std;   
    // don't sync iostreams with printf
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
         po::value<uint32_t>()->default_value(0), 
         "number of in between ASes for ezbgpsec run")
        ("random,b", 
         po::value<bool>()->default_value(true), 
         "disables random tiebraking for testing")
        ("invert-results,i", 
         po::value<bool>()->default_value(true), 
         "record ASNs which do *not* have a route to a prefix-origin")
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
         "name of announcements table")
        ("victim-table,e",
         po::value<string>()->default_value(VICTIM_TABLE),
         "name of victim table")
        ("attacker-table,f",
         po::value<string>()->default_value(ATTACKER_TABLE),
         "name of attacker table")
        ("policy-tables,t",
         po::value< vector<string> >(),
         "space-separated names of ROVpp policy tables")
        ("prop-twice,k",
         po::value<bool>()->default_value(true),
         "flag whether or not to propagate twice")
        ("log-folder,l",
         po::value<string>()->default_value(""),
         "enables the use of logging, best used for debugging only");

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")){
        cout << desc << endl;
        exit(0);
    }
    
    // Handle intro information
    intro();
    
    std::string loggingFolder = vm.count("log-folder") ? vm["log-folder"].as<string>() : "";
    if(loggingFolder != "") {
        if(loggingFolder.back() == '/') {
            Logger::initialize(loggingFolder);
            Logger::getInstance();
        } else {
            std::cerr << "ERROR: Logger Folder must be a directory" << endl;
        }
    }

    // Check for ROV++ mode
    if (vm["rovpp"].as<bool>()) {
         ROVppExtrapolator *extrap = new ROVppExtrapolator(
            (vm.count("policy-tables") ?
                vm["policy-tables"].as< vector<string> >() : 
                vector<string>()),
            vm["random"].as<bool>(),
            (vm.count("results-table") ?
                vm["results-table"].as<string>() :
                RESULTS_TABLE),
            (vm.count("victim-table") ?
                vm["victim-table"].as<string>() : 
                VICTIM_TABLE),
            (vm.count("attacker-table") ?
                vm["attacker-table"].as<string>() : 
                ATTACKER_TABLE),
            (vm["iteration-size"].as<uint32_t>()));
            
        // Run propagation
        bool prop_twice = vm["prop-twice"].as<bool>();
        extrap->perform_propagation(prop_twice);
        // Clean up
        delete extrap;
    } else if(vm["ezbgpsec"].as<uint32_t>()) {
        // Instantiate Extrapolator
        EZExtrapolator *extrap = new EZExtrapolator(vm["random"].as<bool>(),
            vm["invert-results"].as<bool>(),
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
            (vm["iteration-size"].as<uint32_t>()),
            vm["ezbgpsec"].as<uint32_t>(),
            vm["num-in-between"].as<uint32_t>());
            
        // Run propagation
        extrap->perform_propagation();

        // Clean up
        delete extrap;
    } else {
        // Instantiate Extrapolator
        Extrapolator *extrap = new Extrapolator(vm["random"].as<bool>(),
            vm["invert-results"].as<bool>(),
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
            (vm["iteration-size"].as<uint32_t>()));
            
        // Run propagation
        extrap->perform_propagation();
        // Clean up
        delete extrap;
    }

    return 0;
}
#endif // RUN_TESTS
