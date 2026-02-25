#pragma once

#include "atr/graph.hpp"
#include <vector>

namespace atr {

class StaticGraph : public Graph {
public:
  struct BuildEdge {
    NodeID source;
    NodeID target;
    float distance;
    float duration;
  };

  StaticGraph(std::vector<Node> nodes, const std::vector<BuildEdge> &edges);

  size_t nodeCount() const override { return m_nodes.size(); }
  size_t edgeCount() const override { return m_edges.size(); }
  std::span<const Edge> neighbors(NodeID u) const override;
  const Node &nodeDetails(NodeID u) const override;
  NodeID findNearestNode(double lat, double lon) const override;

private:
  std::vector<Node> m_nodes;
  std::vector<Edge> m_edges;
  std::vector<size_t> m_offsets;
};

} // namespace atr
