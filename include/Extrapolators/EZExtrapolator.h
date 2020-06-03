#ifndef EZ_EXTRAPLATOR_H
#define EZ_EXTRAPLATOR_H

#include "Extrapolators/BlockedExtrapolator.h"

#include "SQLQueriers/EZSQLQuerier.h"
#include "Graphs/EZASGraph.h"
#include "Announcements/EZAnnouncement.h"
#include "ASes/EZAS.h"

class EZExtrapolator : public BlockedExtrapolator<EZSQLQuerier, EZASGraph, EZAnnouncement, EZAS> {
public:
    EZExtrapolator(bool random_b=true,
                    std::string r=RESULTS_TABLE,
                    std::string e=VICTIM_TABLE,
                    std::string f=ATTACKER_TABLE,
                    uint32_t iteration_size=false);
                    
    ~EZExtrapolator();
};

#endif