/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

#include "ROVppExtrapolator.h"
#include "ROVppASGraph.h"

ROVppExtrapolator::ROVppExtrapolator(bool invert_results,
                           bool store_depref,
                           std::string a,
                           std::string r,
                           std::string i,
                           std::string d,
                           uint32_t iteration_size)
    : Extrapolator(invert_results, store_depref, a, r, i, d, iteration_size) {
    // Replace the ASGraph with an ROVppASGraph
    // ROVpp specific functions should use the rovpp_graph variable
    // The graph variable maintains backwards compatibility
    // TODO fix this destructor so it doesn't error...
    //delete graph;
    rovpp_graph = new ROVppASGraph();
    graph = rovpp_graph;
}

ROVppExtrapolator::~ROVppExtrapolator() {
}


void ROVppExtrapolator::perform_propagation(bool, size_t) {
  using namespace std;

  // Make tmp directory if it does not exist
  DIR* dir = opendir("/dev/shm/bgp");
  if(!dir){
      mkdir("/dev/shm/bgp", 0777);
  } else {
      closedir(dir);
  }

  // Generate required tables
  if (invert) {
      querier->clear_inverse_from_db();
      querier->create_inverse_results_tbl();
  } else {
      querier->clear_results_from_db();
      querier->create_results_tbl();
  }
  if (depref) {
      querier->clear_depref_from_db();
      querier->create_depref_tbl();
  }
  querier->clear_stubs_from_db();
  querier->create_stubs_tbl();
  querier->clear_non_stubs_from_db();
  querier->create_non_stubs_tbl();
  querier->clear_supernodes_from_db();
  querier->create_supernodes_tbl();

  // Generate the graph and populate the stubs & supernode tables
  graph->create_graph_from_db(querier);

  std::cout << "Generating subnet blocks..." << std::endl;

  // Generate iteration blocks
  std::vector<Prefix<>*> *prefix_blocks = new std::vector<Prefix<>*>; // Prefix blocks
  std::vector<Prefix<>*> *subnet_blocks = new std::vector<Prefix<>*>; // Subnet blocks
  Prefix<> *cur_prefix = new Prefix<>("0.0.0.0", "0.0.0.0"); // Start at 0.0.0.0/0
  populate_blocks(cur_prefix, prefix_blocks, subnet_blocks); // Select blocks based on iteration size
  delete cur_prefix;

  std::cout << "Beginning propagation..." << std::endl;

  // Seed MRT announcements and propagate
  uint32_t announcement_count = 0;
  int iteration = 0;
  auto ext_start = std::chrono::high_resolution_clock::now();

  // For each unprocessed prefix block
  extrapolate_blocks(announcement_count, iteration, false, prefix_blocks);

  // For each unprocessed subnet block
  extrapolate_blocks(announcement_count, iteration, true, subnet_blocks);

  auto ext_finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> e = ext_finish - ext_start;
  std::cout << "Block elapsed time: " << e.count() << std::endl;

  // Cleanup
  delete prefix_blocks;
  delete subnet_blocks;

  std::cout << "Announcement count: " << announcement_count << std::endl;
  std::cout << "Loop count: " << g_loop << std::endl;
  std::cout << "Timestamp Tiebreak count: " << g_ts_tb << std::endl;
  std::cout << "Broken Path count: " << g_broken_path << std::endl;
}

void ROVppExtrapolator::save_results(int iteration) {

}
