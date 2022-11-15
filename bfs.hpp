#pragma once

#include <omp.h>

#include <cstddef>
#include <numeric>
#include <vector>

#include "common.h"
#include "graph.hpp"

constexpr int NOT_VISITED = -1;

/**
 * @brief A more memory-efficient frontier structure
 */
struct Frontier {
  Frontier() = default;
  Frontier(int n) : data_ptr(std::make_shared<int[]>(n)) {
    data     = data_ptr.get();
    capacity = n;
  }
  ~Frontier() = default;

  FORCEINLINE bool empty() { return size == 0; }
  FORCEINLINE void clear() { size = 0; }
  FORCEINLINE void push_back(const int v) { data[size++] = v; }

  std::shared_ptr<int[]> data_ptr;
  int                   *data{nullptr};
  std::size_t            size{0};
  std::size_t            capacity{0};
};

inline void BfsTopDownStep(const Graph &G, Frontier *frontier,
                           Frontier *new_frontier, Frontier *frontiers,
                           Solution &sol) {
  auto     &distance    = sol.distance;
  auto     &parent      = sol.parent;
  const int num_threads = omp_get_max_threads();

#pragma omp parallel for schedule(static)
  for (int i = 0; i < static_cast<int>(frontier->size); ++i) {
    // thread local parameters
    const int tid         = omp_get_thread_num();
    Frontier &pt_frontier = frontiers[tid];
    // expand each node in the previous frontier
    const int  u  = frontier->data[i];
    const int  du = distance[u];
    const int *graph_start =
        G.m_serial_graph.data() + G.m_serial_graph_start[u];

    // TODO: for now directly use the iterator
    for (int j = 0; j < static_cast<int>(G.get_num_edges(u)); ++j) {
      const int v = graph_start[j];
      if (distance[v] == NOT_VISITED &&
          __sync_bool_compare_and_swap(&distance[v], NOT_VISITED, du + 1)) {
        parent[v] = u;
        pt_frontier.push_back(v);
      }
    }
  }

  std::size_t frontier_size[num_threads], frontier_offset[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    const Frontier &pt_frontier = frontiers[i];
    frontier_size[i]      = pt_frontier.size;
  }
  std::exclusive_scan(&frontier_size[0], &frontier_size[0] + num_threads,
                      &frontier_offset[0], 0);
#pragma omp parallel for schedule(static, 1)
  for (int i = 0; i < num_threads; ++i) {
    const Frontier &pt_frontier = frontiers[i];
    std::memcpy(new_frontier->data + frontier_offset[i], pt_frontier.data,
                pt_frontier.size * sizeof(int));
    new_frontier->size += pt_frontier.size;
  }
}

inline std::size_t BfsTopDown(const Graph &G, int source_node, Solution &sol) {
  std::atomic<std::size_t> num_edges = 0;
  // init distance
  auto &distance = sol.distance;
  auto &parent   = sol.parent;
  distance.resize(G.get_num_nodes());
  parent.resize(G.get_num_nodes());
  std::fill(distance.begin(), distance.end(), NOT_VISITED);
  std::fill(parent.begin(), parent.end(), NOT_VISITED);

  // init frontier
  Frontier *frontier, *new_frontier;
  frontier     = new Frontier(G.get_num_nodes() + 1);
  new_frontier = new Frontier(G.get_num_nodes() + 1);

  const int num_threads = omp_get_max_threads();
  Frontier  thread_frontiers[num_threads];
  for (int i = 0; i < num_threads; ++i)
    thread_frontiers[i] = Frontier(G.get_num_nodes());

  frontier->push_back(source_node);
  distance[source_node] = 0;

  while (!frontier->empty()) {
    // traverse the frontier
    new_frontier->clear();

    // The actual step
    BfsTopDownStep(G, frontier, new_frontier, thread_frontiers, sol);

    for (int i = 0; i < num_threads; ++i) thread_frontiers[i].clear();
    // Swap frontiers
    std::swap(frontier, new_frontier);
  }

  delete frontier;
  delete new_frontier;
  return (num_edges + 1) / 2;
}