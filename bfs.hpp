#pragma once

#include <omp.h>

#include <cstddef>
#include <execution>
#include <numeric>
#include <vector>

#include "common.h"
#include "graph.hpp"

#define VERBOSE
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

#pragma omp parallel for schedule(guided)
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

  for (int i = 0; i < num_threads; ++i) {
    const Frontier &pt_frontier = frontiers[i];
    std::memcpy(new_frontier->data + new_frontier->size, pt_frontier.data,
                pt_frontier.size * sizeof(int));
    new_frontier->size += pt_frontier.size;
  }
}

inline void BfsTopDown(const Graph &G, int source_node, Solution &sol) {
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

  int it = 0;
  while (!frontier->empty()) {
    // traverse the frontier
    new_frontier->clear();

#ifdef VERBOSE
    Event top_down_step;
#endif

    // The actual step
    BfsTopDownStep(G, frontier, new_frontier, thread_frontiers, sol);

#ifdef VERBOSE
    auto duration = top_down_step.end();
    Info("{}: {:.4f}", it, duration);
#endif

    for (int i = 0; i < num_threads; ++i) thread_frontiers[i].clear();
    // Swap frontiers
    std::swap(frontier, new_frontier);
    ++it;
  }

  delete frontier;
  delete new_frontier;
}

inline int parallel_collect(int *select, int *out_indices, int n) {
  int  num_not_visited = 0;
  int *select_ps       = new int[n];

  std::exclusive_scan(std::execution::par, select, select + n, select_ps, 0);
  num_not_visited = select_ps[n - 1] + select[n - 1];

#pragma omp parallel for schedule(dynamic, 128)
  for (int i = 0; i < n; ++i) {
    if (select[i] == 1) out_indices[select_ps[i]] = i;
  }

  delete[] select_ps;
  return num_not_visited;
}

inline void BfsBottomUpStep(const Graph &G, Frontier *frontier,
                            Frontier *new_frontier, Frontier *frontiers, int it,
                            Solution &sol) {
  auto &distance = sol.distance;
  auto &parent   = sol.parent;

  int  num_not_visited     = 0;
  int *mark_not_visited    = new int[G.get_num_nodes()];
  int *not_visited_indices = new int[G.get_num_nodes()];

#pragma omp parallel for schedule(dynamic, 128)
  for (int i = 0; i < G.get_num_nodes(); ++i) {
    mark_not_visited[i] = distance[i] == NOT_VISITED ? 1 : 0;
  }

  num_not_visited = parallel_collect(mark_not_visited, not_visited_indices,
                                     G.get_num_nodes());

  int *select = new int[G.get_num_nodes()];
  std::fill(select, select + G.get_num_nodes(), 0);

  // collect all non-visited vertices
#pragma omp parallel for schedule(dynamic, 128)
  for (int i = 0; i < num_not_visited; ++i) {
    const int v = not_visited_indices[i];
    assert(parent[v] == NOT_VISITED);
    // bidirectional graph
    const int *graph_start =
        G.m_serial_graph.data() + G.m_serial_graph_start[v];
    for (int j = 0; j < static_cast<int>(G.get_num_edges(v)); ++j) {
      const int u = graph_start[j];
      // last bfs layer
      if (distance[u] == it) {
        distance[v] = it + 1;
        parent[v]   = u;
        select[v]   = 1;
      }
    }
  }

  new_frontier->size =
      parallel_collect(select, new_frontier->data, G.get_num_nodes());

  delete[] mark_not_visited;
  delete[] not_visited_indices;
  delete[] select;
}

inline void BfsBottomUp(const Graph &G, int source_node, Solution &sol) {
  // init distance
  auto &distance = sol.distance;
  auto &parent   = sol.parent;
  distance.resize(G.get_num_nodes());
  parent.resize(G.get_num_nodes());
  std::fill(distance.begin(), distance.end(), NOT_VISITED);
  std::fill(parent.begin(), parent.end(), NOT_VISITED);

  Frontier *frontier, *new_frontier;
  frontier     = new Frontier(G.get_num_nodes() + 1);
  new_frontier = new Frontier(G.get_num_nodes() + 1);

  const int num_threads = omp_get_max_threads();
  Frontier  thread_frontiers[num_threads];
  for (int i = 0; i < num_threads; ++i)
    thread_frontiers[i] = Frontier(G.get_num_nodes());

  frontier->push_back(source_node);
  distance[source_node] = 0;

  int it = 0;
  while (!frontier->empty()) {
    // traverse the frontier
    new_frontier->clear();

#ifdef VERBOSE
    Event bottom_up_step;
#endif

    // The actual step
    BfsBottomUpStep(G, frontier, new_frontier, thread_frontiers, it, sol);

#ifdef VERBOSE
    auto duration = bottom_up_step.end();
    Info("{}: {:.4f}", it, duration);
#endif

    // Swap frontiers
    for (int i = 0; i < num_threads; ++i) thread_frontiers[i].clear();
    std::swap(frontier, new_frontier);
    ++it;
  }

  delete frontier;
  delete new_frontier;
}

inline void BfsHybrid(const Graph &G, int source_node, Solution &sol) {
  // https://scottbeamer.net/pubs/beamer-sc2012.pdf
}