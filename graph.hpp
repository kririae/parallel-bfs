#pragma once

// clang-format off
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
// clang-format on
#include <vector>

#include "common.h"

class BaseGraph {
public:
  virtual ~BaseGraph()                                   = default;
  FORCEINLINE virtual void        add_edge(int u, int v) = 0;
  FORCEINLINE virtual int         get_num_nodes() const  = 0;
  FORCEINLINE virtual int         get_edge(int u, std::size_t i) const = 0;
  FORCEINLINE virtual std::size_t get_num_edges(int u) const           = 0;
};

/**
 * @brief Naive graph implementation for quick traversal
 */
class LocalGraph : public BaseGraph {
public:
  using graph_type = std::vector<std::vector<int>>;
  LocalGraph()     = default;
  LocalGraph(int n) : m_graph(n + 1) {}
  ~LocalGraph() override = default;
  void post_processing() {
    for (const auto &vertices : m_graph) {
      m_serial_graph_start.push_back(m_serial_graph.size());
      for (const auto &v : vertices) m_serial_graph.push_back(v);
    }
  }

  FORCEINLINE void add_edge(int u, int v) override {
    m_graph[u].push_back(v);
    m_graph[v].push_back(u);
  }
  FORCEINLINE int get_num_nodes() const override { return m_graph.size() - 1; }
  FORCEINLINE int get_edge(int u, std::size_t i) const override {
    return m_serial_graph[m_serial_graph_start[u] + i];
  }

  FORCEINLINE std::size_t get_num_edges(int u) const override {
    return m_graph[u].size();
  }

  graph_type       m_graph;
  std::vector<int> m_serial_graph;
  std::vector<int> m_serial_graph_start;
};

class BoostGraph : public BaseGraph {
public:
  using graph_type =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;

  BoostGraph() = default;
  BoostGraph(int n) : m_graph(n + 1) {}
  ~BoostGraph() override = default;

  FORCEINLINE void add_edge(int u, int v) override {
    boost::add_edge(u, v, m_graph);
  }

  FORCEINLINE int get_num_nodes() const override { return 0; }

  FORCEINLINE int get_edge(int u, std::size_t i) const override {
    assert(false);
  }

  FORCEINLINE std::size_t get_num_edges(int u) const override { assert(false); }

  graph_type m_graph;
};

using Graph = LocalGraph;