#pragma once

#include "atr/ch_graph.hpp"
#include "atr/graph.hpp"
#include <map>

namespace atr {

class CHRouter : public Router {
public:
  CHRouter(const CHGraph &graph);

  std::optional<RouteResult> findRoute(NodeID start, NodeID end,
                                       CostMetric metric) override;

  std::vector<float> findDistances(NodeID start,
                                   const std::vector<NodeID> &targets,
                                   CostMetric metric) override;

private:
  const CHGraph &m_graph;
};

} // namespace atr
