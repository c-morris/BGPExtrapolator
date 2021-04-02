#include "Graphs/ROVASGraph.h"

ROVASGraph::ROVASGraph(bool store_inverse_results, bool store_depref_results) : BaseGraph(store_inverse_results, store_depref_results) {
    attackers = new std::set<uint32_t>();
}
ROVASGraph::ROVASGraph() : ROVASGraph(false, false) {}
ROVASGraph::~ROVASGraph() {
    if (attackers != NULL)
        delete attackers;
}

ROVAS* ROVASGraph::createNew(uint32_t asn) {
    std::cout << "*** create new rov" << std::endl;
    return new ROVAS(asn, attackers, store_depref_results, inverse_results);
}

void ROVASGraph::process(SQLQuerier *querier) {
    // remove_stubs isn't being called
    tarjan();
    combine_components();
    save_supernodes_to_db(querier);
    decide_ranks();
    return;
}

void ROVASGraph::create_graph_from_db(ROVSQLQuerier *querier){
    // Assemble Peers
    pqxx::result R = querier->select_from_table(PEERS_TABLE);
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        add_relationship(c["peer_as_1"].as<uint32_t>(),
                         c["peer_as_2"].as<uint32_t>(),AS_REL_PEER);
        add_relationship(c["peer_as_2"].as<uint32_t>(),
                         c["peer_as_1"].as<uint32_t>(),AS_REL_PEER);
    }
    // Assemble Customer-Providers
    R = querier->select_from_table(CUSTOMER_PROVIDER_TABLE);
    for (pqxx::result::const_iterator c = R.begin(); c!=R.end(); ++c){
        add_relationship(c["customer_as"].as<uint32_t>(),
                         c["provider_as"].as<uint32_t>(),AS_REL_PROVIDER);
        add_relationship(c["provider_as"].as<uint32_t>(),
                         c["customer_as"].as<uint32_t>(),AS_REL_CUSTOMER);
    }

    // Assign the ROV policy to ASes that adopt it
    for (auto policy_table : querier->policy_tables) {
        R = querier->select_AS_flags(policy_table);
        // For each AS in the policy Table
        for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
            // Get the ASN for current AS
            auto search = ases->find(c["asn"].as<uint32_t>());
            if (search != ases->end()) {
                // If this AS adopts the ROV policy, change its adoption parameter to true
                if (c["as_type"].as<uint32_t>() == ROVPPAS_TYPE_ROV)
                    search->second->set_rov_adoption(true);
            }
        }
    }
    process(querier);
    return;
}

void ROVASGraph::add_attacker(uint32_t asn) {
    attackers->insert(asn);
}