#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <assert.h>
#include <random>
#include <map>
#include <typeinfo>
#include <cmath>    // Need?
#include <chrono>   // Need?

#include "SQLQueriers/SQLQuerier.h"
#include "ASes/AS.h"
#include "Graphs/ASGraph.h"
#include "Announcements/Announcement.h"
#include "Extrapolator.h"
#include "Prefix.h"

// Prototypes for PrefixTest.cpp
bool test_prefix();
bool test_string_to_cidr();
bool test_prefix_lt_operator();
bool test_prefix_gt_operator();
bool test_prefix_eq_operator();

// Prototypes for AnnouncementTest.cpp
bool test_announcement();
bool test_ann_lt_operator();
bool test_ann_gt_operator();
bool test_ann_eq_operator();
bool test_to_sql();
bool test_ann_os_operator();
bool test_to_csv();

// Prototypes for ASTest.cpp
bool test_get_random();
bool test_add_neighbor();
bool test_remove_neighbor();
bool test_receive_announcements();
bool test_process_announcement();
bool test_process_announcements();
bool test_already_received();
bool test_clear_announcements();

// Prototypes for ASGraphTest.cpp
bool test_add_relationship();
bool test_translate_asn();
bool test_decide_ranks();
bool test_remove_stubs();
bool test_tarjan();
bool test_combine_components();

// Prototypes for ExtrapolatorTest.cpp
bool test_Extrapolator_constructor();
bool test_propagate_up();
bool test_propagate_down();
bool test_give_ann_to_as_path();
bool test_send_all_announcements();

// Prototypes for SQLQuerierTest.cpp

#endif
