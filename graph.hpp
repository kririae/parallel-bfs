#pragma once

// clang-format off
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
// clang-format on
#include <cstdlib>
#include <fstream>
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
  LocalGraph(int n) : m_graph(n), num_nodes(n) {}
  ~LocalGraph() override = default;

  void post_processing() {
    for (auto &vertices : m_graph) {
      m_serial_graph_start.push_back(m_serial_graph.size());
      m_serial_graph_size.push_back(vertices.size());
      for (const auto &v : vertices) m_serial_graph.push_back(v);
      vertices.clear();
      vertices.shrink_to_fit();  // dealloc memory
    }

    m_graph.clear();
    m_graph.shrink_to_fit();
  }

  FORCEINLINE void add_edge(int u, int v) override {
    m_graph[u].push_back(v);
    m_graph[v].push_back(u);
  }

  FORCEINLINE int get_num_nodes() const override { return num_nodes; }

  FORCEINLINE int get_edge(int u, std::size_t i) const override {
    return m_serial_graph[m_serial_graph_start[u] + i];
  }

  FORCEINLINE std::size_t get_num_edges(int u) const override {
    return m_serial_graph_size[u];
  }

  graph_type       m_graph;
  std::vector<int> m_serial_graph;
  std::vector<int> m_serial_graph_start;
  std::vector<int> m_serial_graph_size;

  std::size_t num_nodes{0};
  std::size_t num_edges{0};
};

class BoostGraph : public BaseGraph {
public:
  using graph_type =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;

  BoostGraph() = default;
  BoostGraph(int n) : m_graph(n) {}
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

inline void GraphFromMM(const std::string &filename, Graph &G) {
  Info("reading {}", filename);
  bool header    = false;
  bool file_info = false;
  int  M, N, L;

  FILE       *ifile = fopen(filename.data(), "r");
  char       *line  = new char[4096];
  int         nread = 0;
  std::size_t n;

  while ((nread = getline(&line, &n, ifile)) != -1) {
    if (n == 1 || line[0] == '%') continue;
    bool header_ = header;
    header       = true;  // trans to header finished
    if (!header_ && header) {
      file_info = true;
      std::sscanf(line, "%d %d %d", &M, &N, &L);
      Info("{} {} {}", M, N, L);
      assert(M == N);
      G = Graph(M + 1);
      continue;
    } else {
      // read common file content
      assert(header);
      assert(file_info);
      int row, column, weight;
      std::sscanf(line, "%d %d %d", &row, &column, &weight);
      assert(weight == 1);
      G.add_edge(row, column);
    }
  }

  Info("{}", G.m_graph.size());
  G.post_processing();
  G.num_edges = L;
  fclose(ifile);
  delete[] line;
}

inline void GraphFromTxt(const std::string &filename, Graph &G,
                         std::vector<std::pair<int, int>> &edges) {
  Info("reading {}", filename);
  FILE       *ifile = fopen(filename.data(), "r");
  char       *line  = new char[4096];
  int         nread = 0;
  std::size_t n;

  int num_vertices = 0;
  while ((nread = getline(&line, &n, ifile)) != -1) {
    if (n == 1 || line[0] == '#') continue;
    int u, v;
    std::sscanf(line, "%d %d", &u, &v);
    edges.emplace_back(std::make_pair(u, v));
    num_vertices = std::max(num_vertices, std::max(u, v));
  }

  fclose(ifile);
  Info("read finished");

  G = Graph(num_vertices + 1);

  Info("adding edges");
  for (const auto &edge : edges) G.add_edge(edge.first, edge.second);

  G.num_edges = edges.size();
  edges.clear();
  edges.shrink_to_fit();

  Info("post processing");
  G.post_processing();
}
