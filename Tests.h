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

#include "SQLQuerier.h"
#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Extrapolator.h"
#include "Prefix.h"

// Prototypes for PrefixTest.cpp
bool test_prefix();
bool test_string_to_cidr();
bool test_prefix_lt_operator();
bool test_prefix_gt_operator();
bool test_prefix_eq_operator();

// Prototypes for ASTest.cpp
bool test_add_neighbor();
bool test_update_rank();
bool test_receive_announcement();
bool test_already_received();
bool test_clear_announcements();
bool test_process_announcements();

#endif
