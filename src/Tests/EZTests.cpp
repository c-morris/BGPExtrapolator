#include "Extrapolators/EZExtrapolator.h"
#include "Announcements/EZAnnouncement.h"

/**
 * 
 *    1
 *    |
 *    2---3---8
 *   /|        \
 *  4 5--6      7
 *
 */
bool testEZExtrapolator() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);
    e.graph->add_relationship(5, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 5, AS_REL_CUSTOMER);
    e.graph->add_relationship(4, 2, AS_REL_PROVIDER);
    e.graph->add_relationship(2, 4, AS_REL_CUSTOMER);

    e.graph->add_relationship(7, 8, AS_REL_PROVIDER);
    e.graph->add_relationship(8, 7, AS_REL_CUSTOMER);

    e.graph->add_relationship(8, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 8, AS_REL_PEER);

    e.graph->add_relationship(2, 3, AS_REL_PEER);
    e.graph->add_relationship(3, 2, AS_REL_PEER);
    
    e.graph->add_relationship(5, 6, AS_REL_PEER);
    e.graph->add_relationship(6, 5, AS_REL_PEER);

    e.graph->decide_ranks();
    Prefix<> p = Prefix<>("137.99.0.0", "255.255.0.0");
    
    EZAnnouncement realAnnouncement = EZAnnouncement(7, p.addr, p.netmask, 300, 7, 0, true, false);
    EZAnnouncement attackAnnouncement = EZAnnouncement(7, p.addr, p.netmask, 299, 7, 0, true, true);

    e.graph->ases->find(7)->second->process_announcement(realAnnouncement, true);
    e.graph->ases->find(4)->second->process_announcement(attackAnnouncement, true);

    e.propagate_up();
    e.propagate_down();

    uint32_t firstASN = e.graph->ases->find(5)->second->all_anns->find(p)->second.received_from_asn;
    uint32_t secondASN = e.graph->ases->find(firstASN)->second->all_anns->find(p)->second.received_from_asn;

    bool from_attacker = e.graph->ases->find(5)->second->all_anns->find(p)->second.from_attacker;

    return secondASN == 4 && from_attacker;
}

bool testDistributeAttackersVictims() {
    EZExtrapolator e = EZExtrapolator();
    e.graph->add_relationship(2, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 2, AS_REL_CUSTOMER);

    e.graph->add_relationship(3, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 3, AS_REL_CUSTOMER);

    e.graph->add_relationship(4, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 4, AS_REL_CUSTOMER);

    e.graph->add_relationship(5, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 5, AS_REL_CUSTOMER);

    e.graph->add_relationship(6, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 6, AS_REL_CUSTOMER);

    e.graph->add_relationship(7, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 7, AS_REL_CUSTOMER);

    e.graph->add_relationship(8, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 8, AS_REL_CUSTOMER);

    e.graph->add_relationship(9, 1, AS_REL_PROVIDER);
    e.graph->add_relationship(1, 9, AS_REL_CUSTOMER);

    e.graph->distributeAttackersVictims(1);

    if(e.graph->destination_victim_to_prefixes->size() != 2 || e.graph->origin_victim_to_attacker->size() != 2)
        return false;

    e.graph->distributeAttackersVictims(.5);

    if(e.graph->destination_victim_to_prefixes->size() != 1 || e.graph->origin_victim_to_attacker->size() != 1)
        return false;

    return true;
}