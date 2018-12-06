#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <assert.h>
#include <random>
#include <map>

#include "SQLQuerier.h"
#include "AS.h"
#include "ASGraph.h"
#include "Announcement.h"
#include "Extrapolator.h"
#include "Prefix.h"

void as_relationship_test();
void tarjan_accuracy_test();
void tarjan_size_test();
void as_receive_test();
void as_process_test();
void set_comparison_test();
void test_db_connection();
void send_all_test();
void tarjan_on_real_data(bool save_large_component = false);
#endif
