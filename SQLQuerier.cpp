#include "SQLQuerier.h"

SQLQuerier::SQLQuerier(){
    //default host and port numbers
    //strings for connection arg
    host = "127.0.0.1";
    port = "5432";

    read_config();
}

//Connects and disconnects from database
void SQLQuerier::test_connection(){

    std::ostringstream stream;
    stream << "dbname = " << db_name;
    stream << " user = " << user;
    stream << " password = " << pass;
    stream << " hostaddr = " << host;
    stream << " port = " << port;

    try {
        
        pqxx::connection C(stream.str());
        if (C.is_open()) {
            std::cout << "Successfully made connection"<< std::endl;
        } else {
            std::cout << "Failed to open database" << std::endl;
            return;
        }
    C.disconnect();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    } 

    return;
}

//Reads credentials/connection info from .conf file
void SQLQuerier::read_config(){
    using namespace std;
    string file_location = "/etc/bgp/bgp_2.conf";
    //Currently uses bgp_2 because pqxx doesn't like 'localhost'
    ifstream cFile(file_location);
    if (cFile.is_open()){
        //map config variables to settings in file
        map<string,string> config;
        string line;
        while(getline(cFile, line)){
            //remove whitespace and check to ignore line
            line.erase(remove_if(line.begin(),line.end(),::isspace), line.end());
            if(line.empty() || line[0] == '#' || line[0] == '['){
                continue;
            }
            auto delim_index = line.find("=");
            std::string var_name = line.substr(0,delim_index);
            std::string value = line.substr(delim_index+1);
            config.insert(std::pair<std::string,std::string>(var_name, value));
        }

        //Add additional config options to this
        for (auto const& setting : config){
            if(setting.first == "username")
                user = setting.second;
            else if(setting.first == "password") 
                pass = setting.second;
            else if(setting.first == "database") 
                db_name = setting.second;
            else if(setting.first == "host") 
                host = setting.second;
            else if(setting.first == "port")
                port = setting.second;
            else{
                std::cerr << "Setting \"" << 
                         setting.first << "\" undefined." << std::endl;
            }
        }    
    }
    else{
        std::cerr << "Error loading config file \"" << file_location << "\"" << std::endl;
    }
}
