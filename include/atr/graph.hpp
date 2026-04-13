#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace atr {

using NodeID = uint32_t;

constexpr auto NO_NODE = std::numeric_limits<NodeID>::max();
constexpr auto INF_DIST = std::numeric_limits<float>::infinity();

struct Node {
  double lat;
  double lon;
  uint64_t osmId;
};

struct Edge {
  NodeID target;
  float distance;
  float duration;
};

enum class CostMetric { Distance, Duration };

class Graph {
public:
  virtual ~Graph() = default;
  virtual size_t nodeCount() const = 0;
  virtual size_t edgeCount() const = 0;
  virtual std::span<const Edge> neighbors(NodeID u) const = 0;
  virtual const Node &nodeDetails(NodeID u) const = 0;
  virtual std::optional<NodeID> findNearestNode(double lat,
                                                double lon) const = 0;
};

struct RouteResult {
  std::vector<NodeID> path;
  float totalCost;
};

class Router {
public:
  virtual ~Router() = default;
  virtual std::optional<RouteResult> findRoute(NodeID start, NodeID end,
                                               CostMetric metric) = 0;
  virtual std::vector<float> findDistances(NodeID start,
                                           const std::vector<NodeID> &targets,
                                           CostMetric metric) = 0;
};

inline std::optional<NodeID> findNearestInNodes(const std::vector<Node> &nodes,
                                                double lat, double lon) {
  if (nodes.empty())
    return std::nullopt;

  NodeID best = 0;
  double minDist = std::numeric_limits<double>::infinity();
  for (size_t i = 0; i < nodes.size(); ++i) {
    double dLat = nodes[i].lat - lat;
    double dLon = nodes[i].lon - lon;
    double distSq = dLat * dLat + dLon * dLon;
    if (distSq < minDist) {
      minDist = distSq;
      best = static_cast<NodeID>(i);
    }
  }
  return best;
}

} // namespace atr
