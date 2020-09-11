#ifndef COMMUNITY_DETECTION_H
#define COMMUNITY_DETECTION_H

#include "Announcements/EZAnnouncement.h"
#include "Graphs/EZASGraph.h"

/**
 *  For EZBGPsec only!
 * 
 *  Community Detection is responsible for collecting the reports from the ASes in the graph to deduce suspect ASes
 *  and remove ASes at the end of the round if they have been proven to be malicious. 
 * 
 *  TODO
 * 
 *  Need a list of adoptors (to cycle through and generate reports)
 *  Need to distrobute adoptor booleans
 * 
 *  Need a quick way to see if an adopting AS sees an attack announcement
 *      - separate list of announcements that get chosen that are pointers to the attacking announcements that got chosen?
 * 
 * G-Bar Requirements:
 *      - Get the degree of any node
 *          - For every node with a degree higher than _
 *      - 
 * 
 */
class CommunityDetection {
public:
    //Naive. Kill any AS that has been in more than threshold attacks
    std::unordered_map<uint32_t, uint32_t>* naive_threshhold_detection;

    int threshold;

    CommunityDetection();
    virtual ~CommunityDetection();

    /**
     * An attacking announcement that an adopting EZBGPsec AS accepted. In theory, this announcement has an invalid MAC for the origin.
     * 
     */
    void add_report(EZAnnouncement& announcement);

    /**
     *  We have deduced the attackers. Exterminate!
     * 
     *  @param graph -> The ASGraph
     */
    void do_real_disconnections(EZASGraph* graph);
};
#endif