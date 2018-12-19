#include "SQLQuerier.h"

SQLQuerier::SQLQuerier(){
    //default host and port numbers
    //strings for connection arg
    host = "127.0.0.1";
    port = "5432";

    read_config();
    open_connection();
}

SQLQuerier::~SQLQuerier(){
    C->disconnect();
    delete C;
}

void SQLQuerier::open_connection(){
    std::ostringstream stream;
    stream << "dbname = " << db_name;
    stream << " user = " << user;
    stream << " password = " << pass;
    stream << " hostaddr = " << host;
    stream << " port = " << port;

    try {
        pqxx::connection *conn = new pqxx::connection(stream.str());
        if (conn->is_open()) {
            std::cout << "Connected to database : " << db_name <<std::endl;
            C = conn;
        }
        else{
            std::cout << "Failed to connect to database : " << db_name <<std::endl;
            return;
        }
    } catch (const std::exception &e){
        std::cerr << e.what() << std::endl;
    }
    return;
}

void SQLQuerier::close_connection(){
    C->disconnect();
}

pqxx::result SQLQuerier::execute(std::string sql){
    try{
        pqxx::nontransaction N(*C);
        pqxx::result R( N.exec(sql));

        return R;

    }
    catch(const std::exception &e){
        std::cerr << e.what() <<std::endl;
    }
}
//TODO maybe rename/overload this for selection options
pqxx::result SQLQuerier::select_from_table(std::string table_name, int limit){
    std::string sql = "SELECT * FROM " + table_name;

    if(limit){
        sql += " LIMIT " + std::to_string(limit);
    }
    return execute(sql);
}

pqxx::result SQLQuerier::select_ann_records(std::string table_name, std::string prefix, int limit){
    std::string sql = "SELECT  host(prefix), netmask(prefix), as_path, next_hop FROM " + table_name;
    if(!prefix.empty()){
        sql += " WHERE prefix = "+ std::string("'") + prefix + std::string("'");
    }
    if(limit){
        sql += " LIMIT " + std::to_string(limit);
    }
    std::cout << sql << std::endl;
    return execute(sql);
}

pqxx::result SQLQuerier::select_distinct_prefixes_from_table(std::string table_name){
    std::string sql = "SELECT DISTINCT prefix, family(prefix) FROM " + table_name;
    return execute(sql);
}

//Reads credentials/connection info from .conf file
void SQLQuerier::read_config(){
    using namespace std;
    string file_location = "/etc/bgp/bgp.conf";
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
            else if(setting.first == "host"){
                if(setting.second == "localhost")
                    host = "127.0.0.1";
                else
                    host = setting.second;
            }
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


