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

inline std::size_t BfsTopDownStep(const Graph &G, Frontier *frontier,
                                  Frontier *new_frontier, Frontier *frontiers,
                                  Solution &sol) {
  std::size_t num_checked_edges = 0;
  auto       &distance          = sol.distance;
  auto       &parent            = sol.parent;
  const int   num_threads       = omp_get_max_threads();

#pragma omp parallel for reduction(+ : num_checked_edges)
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
    num_checked_edges += G.get_num_edges(u);
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

  return num_checked_edges;
}

inline std::size_t BfsTopDown(const Graph &G, int source_node, Solution &sol) {
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

  std::size_t num_checked_edges = 0;

  int it = 0;
  while (!frontier->empty()) {
    // traverse the frontier
    new_frontier->clear();

#ifdef VERBOSE
    Event top_down_step;
#endif

    // The actual step
    num_checked_edges +=
        BfsTopDownStep(G, frontier, new_frontier, thread_frontiers, sol);

#ifdef VERBOSE
    auto duration = top_down_step.end();
    Info("{}: {:.4f} {}", it, duration, num_checked_edges);
#endif

    for (int i = 0; i < num_threads; ++i) thread_frontiers[i].clear();
    // Swap frontiers
    std::swap(frontier, new_frontier);
    ++it;
  }

  delete frontier;
  delete new_frontier;
  return num_checked_edges;
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

inline std::size_t BfsBottomUpStep(const Graph &G, Frontier *frontier,
                                   Frontier *new_frontier, int it,
                                   Solution &sol) {
  std::size_t num_checked_edges = 0;
  auto       &distance          = sol.distance;
  auto       &parent            = sol.parent;

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
#pragma omp parallel for schedule(dynamic, 128) reduction(+ : num_checked_edges)
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
        num_checked_edges += G.get_num_edges(v);
        break;
      }
    }
  }

  new_frontier->size =
      parallel_collect(select, new_frontier->data, G.get_num_nodes());

  delete[] mark_not_visited;
  delete[] not_visited_indices;
  delete[] select;
  return num_checked_edges;
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
    auto num_checked_edges =
        BfsBottomUpStep(G, frontier, new_frontier, it, sol);

#ifdef VERBOSE
    auto duration = bottom_up_step.end();
    Info("{}: {:.4f} {}", it, duration, num_checked_edges);
#endif

    // Swap frontiers
    std::swap(frontier, new_frontier);
    ++it;
  }

  delete frontier;
  delete new_frontier;
}

inline std::size_t BfsHybrid(const Graph &G, int source_node, Solution &sol) {
  // https://scottbeamer.net/pubs/beamer-sc2012.pdf
  // m_f: number of edges from the frontier
  // n_f: number of vertices in the frontier
  // m_u: number of edges to check from unexplored vertices

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
  distance[source_node]           = 0;
  std::size_t   num_checked_edges = 0;
  constexpr int alpha = 14, beta = 24;  // from the paper
  bool          at_top_down = true;

  int it = 0;
  while (!frontier->empty()) {
    // traverse the frontier
    new_frontier->clear();

    // Phase 1: collect all edges
    const auto num_edges = G.num_edges;

    // Phase 2: summing all degrees in frontier
    std::size_t n_f = frontier->size; /* number of vertices in frontier */
    std::size_t m_f = 0;              /* number of edges to check */
    std::size_t m_u = num_edges - num_checked_edges;
#pragma omp parallel for reduction(+ : m_f)
    for (int i = 0; i < frontier->size; ++i)
      m_f += G.get_num_edges(frontier->data[i]);

#ifdef VERBOSE
    Event hybrid_step;
#endif

    // State machine
    if (at_top_down) {
      if (m_f > m_u / alpha) {
        at_top_down = false;
        goto bottom_up_step;
      } else {
        goto top_down_step;
      }
    } else {
      if (n_f < G.get_num_nodes() / beta) {
        at_top_down = true;
        goto top_down_step;
      } else {
        goto bottom_up_step;
      }
    }

    // The actual step
top_down_step:
    num_checked_edges +=
        BfsTopDownStep(G, frontier, new_frontier, thread_frontiers, sol);
    for (int i = 0; i < num_threads; ++i) thread_frontiers[i].clear();
    goto step_end;
bottom_up_step:
    // switch to bottom_up
    num_checked_edges += BfsBottomUpStep(G, frontier, new_frontier, it, sol);
step_end:

#ifdef VERBOSE
    auto duration = hybrid_step.end();
    Info("{} {}: {:.4f}", at_top_down ? "topdown" : "bottomup", it, duration);
#endif

    // Swap frontiers
    std::swap(frontier, new_frontier);
    ++it;
  }

  delete frontier;
  delete new_frontier;
  return num_checked_edges;
}