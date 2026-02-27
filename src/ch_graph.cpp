#include "atr/ch_graph.hpp"
#include <cmath>
#include <limits>

namespace atr {

CHGraph::CHGraph(std::vector<Node> nodes, std::vector<CHEdge> forwardEdges,
                 std::vector<size_t> forwardOffsets,
                 std::vector<CHEdge> backwardEdges,
                 std::vector<size_t> backwardOffsets,
                 std::vector<uint32_t> levels)
    : m_nodes(std::move(nodes)), m_forwardEdges(std::move(forwardEdges)),
      m_forwardOffsets(std::move(forwardOffsets)),
      m_backwardEdges(std::move(backwardEdges)),
      m_backwardOffsets(std::move(backwardOffsets)),
      m_levels(std::move(levels)) {}

std::span<const CHEdge> CHGraph::forwardNeighbors(NodeID u) const {
  if (u >= m_nodes.size())
    return {};
  return {m_forwardEdges.data() + m_forwardOffsets[u],
          m_forwardEdges.data() + m_forwardOffsets[u + 1]};
}

std::span<const CHEdge> CHGraph::backwardNeighbors(NodeID u) const {
  if (u >= m_nodes.size())
    return {};
  return {m_backwardEdges.data() + m_backwardOffsets[u],
          m_backwardEdges.data() + m_backwardOffsets[u + 1]};
}

NodeID CHGraph::findNearestNode(double lat, double lon) const {
  NodeID bestNode = 0;
  double minDist = std::numeric_limits<double>::infinity();

  for (size_t i = 0; i < m_nodes.size(); ++i) {
    double dLat = m_nodes[i].lat - lat;
    double dLon = m_nodes[i].lon - lon;
    double distSq = dLat * dLat + dLon * dLon;
    if (distSq < minDist) {
      minDist = distSq;
      bestNode = static_cast<NodeID>(i);
    }
  }
  return bestNode;
}

} // namespace atr
