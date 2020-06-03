#include "SQLQueriers/EZSQLQuerier.h"

EZSQLQuerier::EZSQLQuerier(std::string r /* = RESULTS_TABLE */,
                            std::string e /* = VICTIM_TABLE */,
                            std::string f /* = ATTACKER_TABLE */) : SQLQuerier() {
    this->results_table = r;
    this->attack_table = f;
    this->victim_table = e;
}

EZSQLQuerier::~EZSQLQuerier() {
    
}