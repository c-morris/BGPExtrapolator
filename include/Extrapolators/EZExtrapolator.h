#ifndef EZ_EXTRAPLATOR_H
#define EZ_EXTRAPLATOR_H

#include "Extrapolators/BlockedExtrapolator.h"

#include "SQLQueriers/EZSQLQuerier.h"
#include "Graphs/EZASGraph.h"
#include "Announcements/EZAnnouncement.h"
#include "ASes/EZAS.h"

class EZExtrapolator : public BlockedExtrapolator<EZSQLQuerier, EZASGraph, EZAnnouncement, EZAS> {
public:
    EZExtrapolator(bool random = true,
                    bool invert_results = true, 
                    bool store_depref = false, 
                    std::string a = ANNOUNCEMENTS_TABLE,
                    std::string r = RESULTS_TABLE, 
                    std::string i = INVERSE_RESULTS_TABLE, 
                    std::string d = DEPREF_RESULTS_TABLE, 
                    uint32_t iteration_size = false);
                    
    ~EZExtrapolator();

    void give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp = 0);
    double percentage_successful_attacks();
};

#endif