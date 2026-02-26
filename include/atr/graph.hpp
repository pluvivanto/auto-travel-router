#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace atr {

using NodeID = uint32_t;

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
  virtual NodeID findNearestNode(double lat, double lon) const = 0;
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

} // namespace atr
