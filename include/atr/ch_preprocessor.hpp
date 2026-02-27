#pragma once

#include "atr/ch_graph.hpp"
#include "atr/static_graph.hpp"
#include <memory>

namespace atr {

class CHPreprocessor {
public:
  static std::unique_ptr<CHGraph> preprocess(const StaticGraph &graph,
                                             CostMetric metric);
};

} // namespace atr
