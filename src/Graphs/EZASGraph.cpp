#include "Graphs/EZASGraph.h"

EZASGraph::EZASGraph() : BaseGraph(false, false) {
}

EZASGraph::~EZASGraph() {

}

EZAS* EZASGraph::createNew(uint32_t asn) {
    return new EZAS(asn, this->max_block_prefix_id);
}

void EZASGraph::process(SQLQuerier<>* querier) {
    //We definately want stubs/edge ASes
    tarjan();
    combine_components();
    //Don't need to save super nodes
    decide_ranks();
    // The EZASGraph must be used with the EZSQLQuerier
    assign_policies(static_cast<EZSQLQuerier*>(querier));
}

void EZASGraph::assign_policies(EZSQLQuerier* querier) {
    for (auto policy_table : querier->policy_tables) {
        pqxx::result R = querier->select_policy_assignments(policy_table);
        // For each AS in the policy Table
        for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
            // Get the ASN for current AS
            auto search = ases->find(c["asn"].as<uint32_t>());
            if (search != ases->end()) {
                // Add the policy to AS
                search->second->policy = c["as_type"].as<uint32_t>();
            }
        }
    }
}
