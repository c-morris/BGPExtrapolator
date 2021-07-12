#include "ASes/EZAS.h"
#include "CommunityDetection.h"
#include "Extrapolators/EZExtrapolator.h"

EZAS::EZAS(CommunityDetection *community_detection, uint32_t asn, uint32_t max_block_prefix_id) : BaseAS<EZAnnouncement>(asn, max_block_prefix_id, false, NULL), community_detection(community_detection) {

}

//EZAS::EZAS(uint32_t asn) : EZAS(NULL, asn, 4) {
//
//}

EZAS::EZAS(uint32_t asn, uint32_t max_block_prefix_id) : BaseAS<EZAnnouncement>(asn, max_block_prefix_id, false, NULL) { }
EZAS::EZAS() : EZAS(0, 20) { }
EZAS::~EZAS() { }

void EZAS::process_announcement(EZAnnouncement &ann, bool ran) {
    //Don't accept if already on the path
    if(std::find(ann.as_path.begin(), ann.as_path.end(), asn) != ann.as_path.end())
        return;

    ann.as_path.insert(ann.as_path.begin(), asn);


    // Reset reserved5, 4 and 5 from any previous modifications
    ann.priority.reserved4 = 0; // default security info missing
    ann.priority.reserved6 = 1; // default valid
    ann.priority.reserved5 = 1; // default short path

    // If origin is only AS on path, accept
    if (ann.origin == asn) {
        BaseAS::process_announcement(ann, ran);
        return;
    }

    // Now deprefer long paths for all policies except BGP
    if (community_detection != NULL) {
        if (policy != EZAS_TYPE_BGP) {
            if (community_detection->extrapolator->round > 1) {
                if (ann.as_path.size() > 5 && 
                    this->prev_anns.find(ann.prefix) != this->prev_anns.end() &&
                    this->prev_anns.find(ann.prefix)->as_path != ann.as_path) {
                    ann.priority.reserved5 = 0;
                } else {
                    ann.priority.reserved5 = 1;
                }
            }
        }
    }
    if(policy == EZAS_TYPE_BGPSEC_TRANSITIVE || policy == EZAS_TYPE_BGPSEC) {
        auto origin_search = community_detection->extrapolator->graph->ases->find(ann.as_path.at(ann.as_path.size() - 1));

        if(origin_search == community_detection->extrapolator->graph->ases->end()) {
            // This indicates invalid input tables
            BOOST_LOG_TRIVIAL(error) << "ORIGIN IN EZAS PATH DOES NOT EXIST!!!";
            exit(10);
        }

        EZAS *origin_as = origin_search->second;
        //Assume that the attacker is causing a missing signature (this is what we are simulating)
        //If this AS is an adopter and there is no signature, deprefer it (security second)
        if(origin_as->policy == EZAS_TYPE_BGPSEC_TRANSITIVE) {
            // path must be longer than 1 here so this is safe
            auto second = ann.as_path.rbegin()[1];
            // If neighbor is not a real neighbor, reject
            std::set<uint32_t> neighbors;
            neighbors.insert(origin_as->providers->begin(), origin_as->providers->end());
            neighbors.insert(origin_as->peers->begin(), origin_as->peers->end());
            neighbors.insert(origin_as->customers->begin(), origin_as->customers->end());
            if (neighbors.find(second) == neighbors.end()) {
                // Not a real neighbor, signature not present
                ann.priority.reserved4 = 0;
            } else {
                // Attacker was able to find a non-adopting neighbor, signature present
                ann.priority.reserved4 = 1;
            }
        } 

        // Regular BGPsec requires a path of un-broken adopters to the origin
        if(origin_as->policy == EZAS_TYPE_BGPSEC) {
            bool contiguous = true;
            for (uint32_t i : ann.as_path) {
                auto search = community_detection->extrapolator->graph->ases->find(i);
                // Assume that an AS that does not exist is not an adopter
                if(search == community_detection->extrapolator->graph->ases->end()) {
                    contiguous = false;
                    break;
                } else {
                    if(search->second->policy != EZAS_TYPE_BGPSEC) {
                        contiguous = false;
                        break;
                    }
                }
            }
            // Use reserved4 field to indicate missing/present signatures
            // Security second (between path length and relationship)
            ann.priority.reserved4 = static_cast<uint8_t>(contiguous);
            if (contiguous && ann.from_attacker) {
                ann.priority.reserved6 = 0; // invalid
            }
        }

        //Now, the announcement has no *visible* invalid relationship

        // Now for both BGPsec variants, do security second
        BaseAS::process_announcement(ann, ran);

        // TODO add this back---it's necessary but also inconsequential for the current simulations
        //// Transitive BGPsec now must decide based on which has more signatures (more adopters on the path)
        //if(policy == EZAS_TYPE_BGPSEC_TRANSITIVE && accepted_announcement.priority / 100 == ann.priority / 100) {
        //    int num_adopters_new = 0;
        //    int num_adopters_accepted = 0;

        //    for (uint32_t i : ann.as_path) {
        //        auto search = community_detection->extrapolator->graph->ases->find(i);
        //        //assume that an AS that does not exist is not an adopter
        //        if(search != community_detection->extrapolator->graph->ases->end()) {
        //            if(search->second->policy == EZAS_TYPE_BGPSEC_TRANSITIVE) {
        //                num_adopters_new++;
        //            }
        //        }
        //    }

        //    for(uint32_t i : accepted_announcement.as_path) {
        //        auto search = community_detection->extrapolator->graph->ases->find(i);
        //        //assume that an AS that does not exist is not an adopter
        //        if(search != community_detection->extrapolator->graph->ases->end()) {
        //            if(search->second->policy == EZAS_TYPE_BGPSEC_TRANSITIVE) {
        //                num_adopters_accepted++;
        //            }
        //        }
        //    }

        //    if(num_adopters_new > num_adopters_accepted) {
        //        ann_search->second = ann;
        //        return;
        //    }
        //}

    } else if(policy == EZAS_TYPE_PATH_END) {
        //If the origin is not actually neighbors with what is on this path, don't consider the announcement
        if(ann.as_path.size() > 2 && !community_detection->extrapolator->graph->ases->at(ann.as_path[ann.as_path.size() - 1])->has_neighbor(ann.as_path.size() - 2))
            return;
    } else if (policy == EZAS_TYPE_COMMUNITY_DETECTION_LOCAL || policy == EZAS_TYPE_DIRECTORY_ONLY) {
        auto path_copy = ann.as_path;
        std::sort(path_copy.begin(), path_copy.end());
        
        // Check for blacklisted paths seen by this AS
        //TODO This could use some improvement
        for(auto &blacklisted_path : community_detection->blacklist) {
            if(std::includes(path_copy.begin(), path_copy.end(), blacklisted_path.begin(), blacklisted_path.end())) {
                //std::cout << "LOC REJECT PATH at " << asn << std::endl;
                return;
            }
        }

        /*
         *  This path does not contain anything that is blacklisted
         *  However, if it is from an attacker, send it to Community Detection
         * 
         *  Do not fret, this is not cheating since CD will check to see if the 
         *      invalid MAC is actually visible.
         * 
         *  In addition, CD will delay adding the hyper edge until the end of the round
         */
        if(ann.from_attacker) {
            community_detection->add_report(ann, community_detection->extrapolator->graph);
        }
    }

    BaseAS::process_announcement(ann, ran);
}

void EZAS::clear_announcements() {
    prev_anns.clear();
    // just download more ram, bro
    prev_anns = *this->all_anns;

    BaseAS::clear_announcements();
}
