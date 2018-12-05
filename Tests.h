#ifndef TESTS_H
#define TESTS_H

#include "SQLQuerier.h"
#include <assert.h>
#include <random>
#include <map>
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
#endif
