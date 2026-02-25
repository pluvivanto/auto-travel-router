#include "atr/static_graph.hpp"
#include <limits>
#include <span>
#include <vector>

namespace atr {

StaticGraph::StaticGraph(std::vector<Node> nodes,
                         const std::vector<BuildEdge> &buildEdges)
    : m_nodes(std::move(nodes)) {
  m_offsets.resize(m_nodes.size() + 1, 0);
  for (const auto &edge : buildEdges) {
    m_offsets[edge.source + 1]++;
  }
  for (size_t i = 0; i < m_nodes.size(); ++i) {
    m_offsets[i + 1] += m_offsets[i];
  }

  m_edges.resize(buildEdges.size());
  std::vector<size_t> currentOffsets = m_offsets;
  for (const auto &edge : buildEdges) {
    m_edges[currentOffsets[edge.source]++] = {edge.target, edge.distance,
                                              edge.duration};
  }
}

std::span<const Edge> StaticGraph::neighbors(NodeID u) const {
  if (u >= m_nodes.size())
    return {};
  return {m_edges.data() + m_offsets[u], m_edges.data() + m_offsets[u + 1]};
}

const Node &StaticGraph::nodeDetails(NodeID u) const { return m_nodes.at(u); }

NodeID StaticGraph::findNearestNode(double lat, double lon) const {
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
