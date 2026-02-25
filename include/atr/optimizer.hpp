#pragma once

#include "atr/graph.hpp"
#include <memory>
#include <vector>

namespace atr {

struct POI {
  NodeID nodeId;
  double lat;
  double lon;
  POI() = default;
  POI(NodeID id, double lt, double ln) : nodeId(id), lat(lt), lon(ln) {}
};

struct DailyItinerary {
  std::vector<NodeID> sequence;
  std::vector<NodeID> fullPath;
};

struct OptimizationResult {
  std::vector<DailyItinerary> days;
};

class Optimizer {
public:
  Optimizer(Graph &graph, Router &router);

  OptimizationResult optimize(const NodeID hotelNode,
                              const std::vector<POI> &pois, int numDays,
                              CostMetric metric = CostMetric::Duration);

private:
  struct Cluster {
    std::vector<POI> pois;
    double centerLat;
    double centerLon;
  };

  std::vector<Cluster> clusterPois(const std::vector<POI> &pois, int k);
  DailyItinerary optimizeDay(NodeID hotel, const std::vector<POI> &dayPois,
                              CostMetric metric);

  Router &m_router;
};

} // namespace atr
