#include "ASes/EZAS.h"
#include "CommunityDetection.h"
#include "Extrapolators/EZExtrapolator.h"

EZAS::EZAS(CommunityDetection *community_detection, uint32_t asn) : BaseAS<EZAnnouncement>(asn, false, NULL), community_detection(community_detection) {

}

EZAS::EZAS(uint32_t asn) : EZAS(NULL, asn) {

}

EZAS::EZAS() : EZAS(0) { }
EZAS::~EZAS() { }

void EZAS::process_announcement(EZAnnouncement &ann, bool ran) {
    //Don't accept if already on the path
    if(std::find(ann.as_path.begin(), ann.as_path.end(), asn) != ann.as_path.end())
        return;

    ann.as_path.insert(ann.as_path.begin(), asn);

    // If origin is only AS on path, accept
    if (ann.origin == asn) {
        BaseAS::process_announcement(ann, ran);
        return;
    }

    if(policy == EZAS_TYPE_BGPSEC_TRANSITIVE || policy == EZAS_TYPE_BGPSEC) {
        bool signature_missing = false;
        auto origin_search = community_detection->extrapolator->graph->ases->find(ann.as_path.at(ann.as_path.size() - 1));

        if(origin_search == community_detection->extrapolator->graph->ases->end()) {
            // This indicates invalid input tables
            std::cerr << "ORIGIN IN EZAS PATH DOES NOT EXIST!!!" << std::endl;
            return;
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
                signature_missing = true;
            //} else {
                // Attacker was able to find a non-adopting neighbor, signature present
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
            if (!contiguous) {
                // Process announcement normally
                //BaseAS::process_announcement(ann, ran);
                //return;
                signature_missing = true;
            } else if (contiguous && ann.from_attacker) {
                signature_missing = true;
            }
        }

        //Now, the announcement has no *visible* invalid relationship

        //Accept if there is nothing to compete
        auto ann_search = all_anns->find(ann.prefix);
        if(ann_search == all_anns->end()) {
            all_anns->insert(std::make_pair(ann.prefix, ann));
            return;
        }

        EZAnnouncement &accepted_announcement = ann_search->second;
        bool accepted_signature_missing = false;

        // BGPsec now needs to check security second with the existing announcement
        if(origin_as->policy == EZAS_TYPE_BGPSEC) {
            bool contiguous = true;
            for (uint32_t i : accepted_announcement.as_path) {
                auto search = community_detection->extrapolator->graph->ases->find(i);
                // assume that an AS that does not exist is not an adopter
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
            accepted_signature_missing = !contiguous;
        }

        // Transitive BGPsec does something similar
        if(origin_as->policy == EZAS_TYPE_BGPSEC_TRANSITIVE) {
            // path must be longer than 1 here so this is safe
            auto accepted_second = accepted_announcement.as_path.rbegin()[1];
            // If neighbor is not a real neighbor, reject
            std::set<uint32_t> accepted_neighbors;
            accepted_neighbors.insert(origin_as->providers->begin(), origin_as->providers->end());
            accepted_neighbors.insert(origin_as->peers->begin(), origin_as->peers->end());
            accepted_neighbors.insert(origin_as->customers->begin(), origin_as->customers->end());
            if (accepted_neighbors.find(accepted_second) == accepted_neighbors.end()) {
                // Not a real neighbor, signature not present
                accepted_signature_missing = true;
            }
        } 

        // Now for both BGPsec variants, do security second
        if (accepted_announcement.priority / 100 < ann.priority / 100) {
            // Relationship first
            BaseAS::process_announcement(ann, ran);
            return;
        }
        if (accepted_announcement.priority / 100 == ann.priority / 100) {
            // If relation equal but security better on new ann
            if (!signature_missing && accepted_signature_missing) {
                ann_search->second = ann;
                return;
            }
            // If relation equal but security is worse, reject
            if (signature_missing && !accepted_signature_missing) {
                return;
            }
            // Else let process_announcement sort them out
            BaseAS::process_announcement(ann, ran);
            return;
        }

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

        ////If security is the same, then compare based on priority
        //if(ann.priority > accepted_announcement.priority) {
        //    ann_search->second = ann;
        //}
        //return;
    } else if (community_detection != NULL) {
        auto path_copy = ann.as_path;
        std::sort(path_copy.begin(), path_copy.end());
        if (policy == EZAS_TYPE_COMMUNITY_DETECTION_LOCAL || policy == EZAS_TYPE_DIRECTORY_ONLY) {
            // Check for blacklisted paths seen by this AS
            //TODO This could use some improvement
            for(auto &blacklisted_path : community_detection->blacklist) {
                if(std::includes(path_copy.begin(), path_copy.end(), blacklisted_path.begin(), blacklisted_path.end())) {
                    //std::cout << "LOC REJECT PATH at " << asn << std::endl;
                    return;
                }
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
