#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "bfs.hpp"
#include "common.h"
#include "graph.hpp"

#undef NDEBUG

Graph G;

int main(int argc, char **argv) {
  spdlog::set_pattern("\% %v");
  // Although the input graph is directed, we'll treat is as undirected graph to
  // traverse.

  // reference: https://math.nist.gov/MatrixMarket/mmio/c/example_read.c
  if (argc != 4) {
    Error("bfs [source_node] [graph_file].mm [omp_num_threads]");
    exit(-1);
  }

  const int source_node = std::atoi(argv[1]);
  Info("reading mm");

  std::string   line;
  std::ifstream file(argv[2]);

  const int num_threads = std::atoi(argv[3]);
  omp_set_dynamic(0);
  omp_set_num_threads(num_threads);

  bool header    = false;
  bool file_info = false;
  int  M, N, L;

  while (std::getline(file, line)) {
    if (line.starts_with('%') || line.empty()) continue;
    bool header_ = header;
    header       = true;  // trans to header finished
    if (!header_ && header) {
      file_info = true;
      std::sscanf(line.data(), "%d %d %d", &M, &N, &L);
      assert(M == N);
      G = Graph(M + 1);
      continue;
    } else {
      // read common file content
      assert(header);
      assert(file_info);
      int row, column, weight;
      std::sscanf(line.data(), "%d %d %d", &row, &column, &weight);
      G.add_edge(row, column);
      assert(weight == 1);
    }
  }

  G.post_processing();

  Solution sol;
  Event    bfs_event;
  auto     num_edges  = BfsTopDown(G, source_node, sol);
  num_edges           = L;
  const auto exe_time = bfs_event.end();
  Info("Time: {} ms", exe_time);
  Info("num_nodes: {}", G.get_num_nodes());
  Info("num_edges: {}", num_edges);
  const auto MTEPS = num_edges / (exe_time * 1e6 * 1e-3);
  Info("MTEPS: {:.4f}", MTEPS);
  printf("%.4f %.4f\n", exe_time, MTEPS);
#if 0
  for (std::size_t i = 1; i < sol.distance.size(); ++i) 
    std::cout << sol.distance[i] << std::endl;
#endif

  file.close();
}