#ifndef ROVAS_H
#define ROVAS_H

#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"

struct ROVAS: public AS {
  std::uint32_t attacker_asn;
  std::uint32_t victim_asn;
  std::string victim_prefix;
  std::map<Prefix<>, Announcement> blocked_map;
  ASGraph *as_graph;

  ROVAS(uint32_t myasn=0,
      std::uint32_t attacker_asn=0,
      std::uint32_t victim_asn=1,
      std::string victim_prefix=NULL,
      std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inv=NULL,
      ASGraph *graph=NULL,
      std::set<uint32_t> *prov=NULL,
      std::set<uint32_t> *peer=NULL,
      std::set<uint32_t> *cust=NULL);
  ~ROVAS();
  void add_neighbor(uint32_t asn, int relationship);
  void remove_neighbor(uint32_t asn, int relationship);
  void receive_announcement(Announcement &ann);
  void clear_announcements();
  bool already_received(Announcement &ann);
  void printDebug();
  void process_announcements();
  friend std::ostream& operator<<(std::ostream &os, const AS& as);
  std::ostream& stream_announcements(std:: ostream &os);
  void receive_announcements(std::vector<Announcement> &announcements);

  // ROV Methods
  bool pass_rov(Announcement &ann);
};

#endif
