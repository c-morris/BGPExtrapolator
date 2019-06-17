#ifndef ROVPPAS_H
#define ROVPPAS_H

#include "ROVAS.h"
#include "NegativeAnnouncement.h"


struct ROVppAS: public ROVAS {
  std::set<NegativeAnnouncement> negative_announcements;
  std::map<Prefix<>, Announcement> blackhole_map;

  ROVppAS(uint32_t myasn=0,
      std::uint32_t attacker_asn=0,
      std::uint32_t victim_asn=1,
      std::string victim_prefix=NULL,
      std::map<std::pair<Prefix<>, uint32_t>,std::set<uint32_t>*> *inv=NULL,
      ASGraph *graph=NULL,
      std::set<uint32_t> *prov=NULL,
      std::set<uint32_t> *peer=NULL,
      std::set<uint32_t> *cust=NULL) : ROVAS(myasn, attacker_asn, victim_asn,
      victim_prefix, inv, graph, prov, peer, cust) {
        negative_announcements = std::set<NegativeAnnouncement>();
        blackhole_map = std::map<Prefix<>, Announcement>();
      }
  ~ROVppAS();
  // Overrided AS Methods
  virtual void receive_announcements(std::vector<Announcement> &announcements);
  virtual void receive_announcement(Announcement &ann);

  // ROV Methods
  bool pass_rov(Announcement &ann);

  // ROVpp methods
  void make_negative_announcement_and_blackhole(Announcement &legit_ann, Announcement &hijacked_ann);
  std::vector<uint32_t> get_as_path(Announcement &ann);
  bool paths_intersect(Announcement &legit_ann, Announcement &hijacked_ann);
  std::pair<bool, Announcement*> received_valid_announcement(Announcement &announcement);
  std::pair<bool, Announcement*> received_hijack_announcement(Announcement &announcement);
  std::ostream& stream_blacklist(std::ostream &os);
  void incoming_negative_announcement(Announcement &ann);
  void incoming_valid_announcement(Announcement &ann);
  void incoming_hijack_announcement(Announcement &ann);
};

#endif
