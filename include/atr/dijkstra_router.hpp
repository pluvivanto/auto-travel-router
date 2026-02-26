#pragma once

#include "atr/graph.hpp"
#include <optional>

namespace atr {

class DijkstraRouter : public Router {
public:
  DijkstraRouter(const Graph &graph);
  std::optional<RouteResult> findRoute(NodeID start, NodeID end,
                                       CostMetric metric) override;
  std::vector<float> findDistances(NodeID start,
                                   const std::vector<NodeID> &targets,
                                   CostMetric metric) override;

private:
  const Graph &m_graph;
};

} // namespace atr
