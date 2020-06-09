#include "Extrapolators/EZExtrapolator.h"

EZExtrapolator::EZExtrapolator(bool random /* = true */,
                 bool invert_results /* = true */, 
                 bool store_depref /* = false */, 
                 std::string a /* = ANNOUNCEMENTS_TABLE */,
                 std::string r /* = RESULTS_TABLE */, 
                 std::string i /* = INVERSE_RESULTS_TABLE */, 
                 std::string d /* = DEPREF_RESULTS_TABLE */, 
                 uint32_t iteration_size /* = false */) : BlockedExtrapolator(random, invert_results, store_depref, a, r, i, d, iteration_size) {
    
    graph = new EZASGraph();
    querier = new EZSQLQuerier(a, r, i, d);

    total_attacks = 0;
    successful_attacks = 0;
}

EZExtrapolator::~EZExtrapolator() {

}

void EZExtrapolator::give_ann_to_as_path(std::vector<uint32_t>* as_path, Prefix<> prefix, int64_t timestamp /* = 0 */) {
    uint32_t path_origin_asn = as_path->at(as_path->size() - 1);

    auto result = graph->origin_victim_to_attacker->find(path_origin_asn);

    //Test if this is an attack
    if(result == graph->origin_victim_to_attacker->end()) {
        //Just run as usual then
        BlockedExtrapolator::give_ann_to_as_path(as_path, prefix, timestamp);
        return;
    }

    uint32_t attacker_asn = result->second.first;
    uint32_t victim2_asn = result->second.second;

    if(graph->destination_victim_to_prefixes->find(victim2_asn) != graph->destination_victim_to_prefixes->end()) {
        //only one attacking announcement per attacker, this way the metric is the percent of successful and *unique* attacks
        BlockedExtrapolator::give_ann_to_as_path(as_path, prefix, timestamp);
        return;
    }

    graph->destination_victim_to_prefixes->insert({ victim2_asn, prefix });

    EZAS* victim1 = this->graph->ases->find(path_origin_asn)->second;
    EZAS* attacker = this->graph->ases->find(attacker_asn)->second;

    //Create the "real" and "fake" announcement. These two will "compete" in propagation, and we want to see which ends up in the all_anns for victim 2
    EZAnnouncement realAnnouncement = EZAnnouncement(path_origin_asn, prefix.addr, prefix.netmask, 300, path_origin_asn, timestamp, true, false);
    EZAnnouncement attackAnnouncement = EZAnnouncement(path_origin_asn, prefix.addr, prefix.netmask, 299, path_origin_asn, timestamp, true, true);

    //Check if already recieved?
    victim1->process_announcement(realAnnouncement, this->random);
    attacker->process_announcement(attackAnnouncement, this->random);
}

/*
 * This will update the global variables thatrepresent the statistical information
 * This runs at the end of every iteration
 */
void EZExtrapolator::calculate_sum_successful_attacks() {
    for(auto& it : *graph->destination_victim_to_prefixes) {
        uint32_t asn = it.first;
        EZAS* as = graph->ases->find(asn)->second;

        auto result = as->all_anns->find(it.second);

        if(result == as->all_anns->end()) {
            continue;
        }

        if(result->second.from_attacker)
            successful_attacks++;

        total_attacks++;
    }

    graph->destination_victim_to_prefixes->clear();
}

void EZExtrapolator::save_results(int iteration) {
    //Only interested in the probibility
    // BaseExtrapolator::save_results(iteration);
    calculate_sum_successful_attacks();
}