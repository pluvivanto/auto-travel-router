#pragma once

#include "atr/graph.hpp"
#include <limits>
#include <span>
#include <vector>

namespace atr {

struct CHEdge {
  NodeID target;
  float weight;
  NodeID skippedNode; // std::numeric_limits<NodeID>::max() if not a shortcut
};

class CHGraph : public Graph {
public:
  CHGraph(std::vector<Node> nodes, std::vector<CHEdge> forwardEdges,
          std::vector<size_t> forwardOffsets, std::vector<CHEdge> backwardEdges,
          std::vector<size_t> backwardOffsets, std::vector<uint32_t> levels);

  size_t nodeCount() const override { return m_nodes.size(); }
  size_t edgeCount() const override { return m_forwardEdges.size(); }
  std::span<const Edge> neighbors(NodeID u) const override {
    (void)u;
    return {};
  }
  const Node &nodeDetails(NodeID u) const override { return m_nodes[u]; }
  NodeID findNearestNode(double lat, double lon) const override;

  std::span<const CHEdge> forwardNeighbors(NodeID u) const;
  std::span<const CHEdge> backwardNeighbors(NodeID u) const;
  uint32_t level(NodeID u) const { return m_levels[u]; }

private:
  std::vector<Node> m_nodes;
  std::vector<CHEdge> m_forwardEdges;
  std::vector<size_t> m_forwardOffsets;
  std::vector<CHEdge> m_backwardEdges;
  std::vector<size_t> m_backwardOffsets;
  std::vector<uint32_t> m_levels;
};

} // namespace atr
