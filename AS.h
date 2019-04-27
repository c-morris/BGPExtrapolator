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
    uint32_t asn;
    bool visited;
    int rank;
    // converted these from map<Prefix, Announcement> to vector<Announcement>
    std::vector<Announcement> *anns_sent_to_peers_providers;
    std::vector<Announcement> *incoming_announcements;
    // this one stays as a map
    std::map<Prefix<>, Announcement> *all_anns;
    // these should not need to be ordered
    std::set<uint32_t> *providers; 
    std::set<uint32_t> *peers; 
    std::set<uint32_t> *customers; 
    std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inverse_results; 
    //If this AS represents multiple ASes, it's "members" are listed here
    std::vector<uint32_t> *member_ases;

    // these are assigned and used in Tarjan's algorithm
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
    void receive_announcements(std::vector<Announcement> &announcements);
    void receive_announcement(Announcement &ann);
    void clear_announcements();
    bool update_rank(int newrank);
    bool already_received(Announcement &ann);
    void printDebug();
    void process_announcements();
    friend std::ostream& operator<<(std::ostream &os, const AS& as);
    std::ostream& pandas_stream_announcements(std:: ostream &os);
    std::ostream& stream_announcements(std:: ostream &os);
};

#endif
