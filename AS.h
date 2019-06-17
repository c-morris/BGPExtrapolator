#ifndef AS_H
#define AS_H

#define AS_REL_PROVIDER 0
#define AS_REL_PEER 1
#define AS_REL_CUSTOMER 2

#include <string>
#include <set>
#include <map>
#include <vector>
#include <iostream>
#include "Announcement.h"
#include "Prefix.h"

struct AS {
    uint32_t asn;       // Autonomous System Number
    bool visited;       // Marks something
    int rank;           // Rank in ASGraph heirarchy for propagation
    
    // Defer processing of incoming announcements for efficiency
    std::vector<Announcement> *incoming_announcements;
    // This vector is for something?
    std::vector<Announcement> *anns_sent_to_peers_providers;
    // Map of all announcements stored
    std::map<Prefix<>, Announcement> *all_anns;
    std::map<Prefix<>, Announcement> *depref_anns;
    // AS Relationships
    std::set<uint32_t> *providers; 
    std::set<uint32_t> *peers; 
    std::set<uint32_t> *customers; 
    // Pointer to inverted results map for efficiency
    std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results; 
    // If this AS represents multiple ASes, it's "members" are listed here (Supernodes)
    std::vector<uint32_t> *member_ases;
    // Assigned and used in Tarjan's algorithm
    int index;
    int lowlink;
    bool onStack;

    AS(uint32_t myasn=0, 
        std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results=NULL,
        std::set<uint32_t> *prov=NULL, 
        std::set<uint32_t> *peer=NULL,
        std::set<uint32_t> *cust=NULL);
    ~AS();
    
    void add_neighbor(uint32_t asn, int relationship);
    void remove_neighbor(uint32_t asn, int relationship);
    void receive_announcements(std::vector<Announcement> &announcements);
    void receive_announcement(Announcement &ann);
    void clear_announcements();
    bool already_received(Announcement &ann);
    void printDebug();
    void process_announcements();
    void swap_inverse_result(std::pair<Prefix<>,uint32_t> old, 
                             std::pair<Prefix<>,uint32_t> current);
    friend std::ostream& operator<<(std::ostream &os, const AS& as);
    std::ostream& stream_announcements(std:: ostream &os);
    std::ostream& stream_depref(std:: ostream &os);
};

#endif
