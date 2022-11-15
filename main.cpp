#include <cstdio>
#include <iostream>
#include <string>

#include "bfs.hpp"
#include "common.h"
#include "graph.hpp"

#undef NDEBUG

Graph G;

// global edges
std::vector<std::pair<int, int>> edges;

int main(int argc, char **argv) {
  spdlog::set_pattern("\% %v");
  // Although the input graph is directed, we'll treat is as undirected graph to
  // traverse.

  // reference: https://math.nist.gov/MatrixMarket/mmio/c/example_read.c
  if (argc != 5) {
    Error(
        "bfs [source_node] [graph_file].[mm|txt] [omp_num_threads] "
        "[bfs_method]");
    exit(-1);
  }

  const int source_node = std::atoi(argv[1]);
  const int num_threads = std::atoi(argv[3]);
  omp_set_dynamic(0);
  omp_set_num_threads(num_threads);

  const int bfs_method = std::atoi(argv[4]);

  const auto filename = std::string(argv[2]);
  if (filename.ends_with(".mm")) {
    GraphFromMM(argv[2], G);
  } else if (filename.ends_with(".txt")) {
    GraphFromTxt(argv[2], G, edges);
  } else {
    Error("filename suffix not matched");
    exit(-1);
  }

  Solution sol;
  Event    bfs_event;

  switch (bfs_method) {
    case 0:
      BfsTopDown(G, source_node, sol);
      break;
    case 1:
      BfsBottomUp(G, source_node, sol);
      break;
    default:
      Error("no bfs method exists");
      exit(-1);
  }

  auto       num_edges = G.num_edges;
  const auto exe_time  = bfs_event.end();
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
}