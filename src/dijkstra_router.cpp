#include "atr/dijkstra_router.hpp"
#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

namespace atr {

DijkstraRouter::DijkstraRouter(const Graph &graph) : m_graph(graph) {}

std::optional<RouteResult> DijkstraRouter::findRoute(NodeID start, NodeID end,
                                                     CostMetric metric) {
  size_t n = m_graph.nodeCount();
  std::vector<float> distances(n, std::numeric_limits<float>::infinity());
  std::vector<NodeID> predecessors(n, std::numeric_limits<NodeID>::max());

  using State = std::pair<float, NodeID>;
  std::priority_queue<State, std::vector<State>, std::greater<State>> pq;

  distances[start] = 0.0f;
  pq.push({0.0f, start});

  while (!pq.empty()) {
    auto [d, u] = pq.top();
    pq.pop();

    if (d > distances[u])
      continue;
    if (u == end)
      break;

    for (const auto &edge : m_graph.neighbors(u)) {
      float weight =
          (metric == CostMetric::Distance) ? edge.distance : edge.duration;
      if (distances[u] + weight < distances[edge.target]) {
        distances[edge.target] = distances[u] + weight;
        predecessors[edge.target] = u;
        pq.push({distances[edge.target], edge.target});
      }
    }
  }

  if (distances[end] == std::numeric_limits<float>::infinity()) {
    return std::nullopt;
  }

  std::vector<NodeID> path;
  for (NodeID at = end; at != std::numeric_limits<NodeID>::max();
       at = predecessors[at]) {
    path.push_back(at);
  }
  std::reverse(path.begin(), path.end());

  return RouteResult{std::move(path), distances[end]};
}

std::vector<float>
DijkstraRouter::findDistances(NodeID start, const std::vector<NodeID> &targets,
                              CostMetric metric) {
  size_t n = m_graph.nodeCount();
  std::vector<float> distances(n, std::numeric_limits<float>::infinity());
  std::vector<float> result(targets.size(),
                            std::numeric_limits<float>::infinity());

  std::vector<std::vector<size_t>> targetMap(n);
  size_t targetsToFind = 0;
  for (size_t i = 0; i < targets.size(); ++i) {
    if (targets[i] < n) {
      if (targetMap[targets[i]].empty()) {
        targetsToFind++;
      }
      targetMap[targets[i]].push_back(i);
    }
  }

  if (targetsToFind == 0)
    return result;

  using State = std::pair<float, NodeID>;
  std::priority_queue<State, std::vector<State>, std::greater<State>> pq;

  distances[start] = 0.0f;
  pq.push({0.0f, start});

  size_t settledTargets = 0;

  while (!pq.empty() && settledTargets < targetsToFind) {
    auto [d, u] = pq.top();
    pq.pop();

    if (d > distances[u])
      continue;

    if (!targetMap[u].empty()) {
      for (size_t targetIdx : targetMap[u]) {
        result[targetIdx] = d;
      }
      settledTargets++;
    }

    for (const auto &edge : m_graph.neighbors(u)) {
      float weight =
          (metric == CostMetric::Distance) ? edge.distance : edge.duration;
      if (distances[u] + weight < distances[edge.target]) {
        distances[edge.target] = distances[u] + weight;
        pq.push({distances[edge.target], edge.target});
      }
    }
  }

  return result;
}

} // namespace atr
