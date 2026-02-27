#include "atr/ch_router.hpp"
#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

namespace atr {

CHRouter::CHRouter(const CHGraph &graph) : m_graph(graph) {}

struct SearchState {
  float dist;
  NodeID node;
  bool operator>(const SearchState &other) const { return dist > other.dist; }
};

struct EdgeInfo {
  NodeID from;
  NodeID skipped;
  bool isForward;
};

std::optional<RouteResult> CHRouter::findRoute(NodeID start, NodeID end,
                                               CostMetric metric) {
  (void)metric;
  if (start == end)
    return RouteResult{{start}, 0.0f};

  size_t n = m_graph.nodeCount();
  std::vector<float> distF(n, std::numeric_limits<float>::infinity());
  std::vector<float> distB(n, std::numeric_limits<float>::infinity());
  std::vector<EdgeInfo> parentF(n, {std::numeric_limits<NodeID>::max(),
                                    std::numeric_limits<NodeID>::max(), true});
  std::vector<EdgeInfo> parentB(n, {std::numeric_limits<NodeID>::max(),
                                    std::numeric_limits<NodeID>::max(), false});

  std::priority_queue<SearchState, std::vector<SearchState>, std::greater<>>
      pqF, pqB;

  distF[start] = 0.0f;
  pqF.push({0.0f, start});
  distB[end] = 0.0f;
  pqB.push({0.0f, end});

  float bestDist = std::numeric_limits<float>::infinity();
  NodeID meetingNode = std::numeric_limits<NodeID>::max();

  std::vector<NodeID> visitedF, visitedB;

  while (!pqF.empty() || !pqB.empty()) {
    if (!pqF.empty()) {
      auto [d, u] = pqF.top();
      pqF.pop();
      if (d > bestDist && (!pqB.empty() && d > pqB.top().dist)) {
        // Can't improve
      } else if (d <= distF[u]) {
        visitedF.push_back(u);
        for (const auto &e : m_graph.forwardNeighbors(u)) {
          float newDist = d + e.weight;
          if (newDist < distF[e.target]) {
            distF[e.target] = newDist;
            parentF[e.target] = {u, e.skippedNode, true};
            pqF.push({newDist, e.target});
            if (distB[e.target] != std::numeric_limits<float>::infinity()) {
              if (newDist + distB[e.target] < bestDist) {
                bestDist = newDist + distB[e.target];
                meetingNode = e.target;
              }
            }
          }
        }
      }
    }

    if (!pqB.empty()) {
      auto [d, u] = pqB.top();
      pqB.pop();
      if (d > bestDist && (!pqF.empty() && d > pqF.top().dist)) {
      } else if (d <= distB[u]) {
        visitedB.push_back(u);
        for (const auto &e : m_graph.backwardNeighbors(u)) {
          float newDist = d + e.weight;
          if (newDist < distB[e.target]) {
            distB[e.target] = newDist;
            parentB[e.target] = {u, e.skippedNode, false};
            pqB.push({newDist, e.target});
            if (distF[e.target] != std::numeric_limits<float>::infinity()) {
              if (newDist + distF[e.target] < bestDist) {
                bestDist = newDist + distF[e.target];
                meetingNode = e.target;
              }
            }
          }
        }
      }
    }
  }

  if (bestDist == std::numeric_limits<float>::infinity())
    return std::nullopt;

  std::vector<NodeID> path;

  auto unpack = [&](auto self, NodeID u, NodeID v, NodeID skipped) -> void {
    if (skipped == std::numeric_limits<NodeID>::max()) {
      path.push_back(v);
    } else {
      NodeID w = skipped;

      NodeID sw = std::numeric_limits<NodeID>::max();
      if (m_graph.level(w) > m_graph.level(u)) {
        for (auto &e : m_graph.forwardNeighbors(u))
          if (e.target == w) {
            sw = e.skippedNode;
            break;
          }
      } else {
        for (auto &e : m_graph.backwardNeighbors(w))
          if (e.target == u) {
            sw = e.skippedNode;
            break;
          }
      }
      self(self, u, w, sw);

      NodeID wv = std::numeric_limits<NodeID>::max();
      if (m_graph.level(v) > m_graph.level(w)) {
        for (auto &e : m_graph.forwardNeighbors(w))
          if (e.target == v) {
            wv = e.skippedNode;
            break;
          }
      } else {
        for (auto &e : m_graph.backwardNeighbors(v))
          if (e.target == w) {
            wv = e.skippedNode;
            break;
          }
      }
      self(self, w, v, wv);
    }
  };

  path.push_back(start);

  // Backtrack from meetingNode to start
  std::vector<NodeID> forwardPart;
  NodeID curr = meetingNode;
  while (curr != start) {
    forwardPart.push_back(curr);
    curr = parentF[curr].from;
  }
  std::reverse(forwardPart.begin(), forwardPart.end());

  curr = start;
  for (NodeID next : forwardPart) {
    unpack(unpack, curr, next, parentF[next].skipped);
    curr = next;
  }

  // Backtrack from meetingNode to end
  std::vector<NodeID> backwardPart;
  curr = meetingNode;
  while (curr != end) {
    NodeID next = parentB[curr].from;
    backwardPart.push_back(next);
    curr = next;
  }

  curr = meetingNode;
  for (NodeID next : backwardPart) {
    unpack(unpack, curr, next, parentB[curr].skipped);
    curr = next;
  }

  return RouteResult{std::move(path), bestDist};
}

std::vector<float> CHRouter::findDistances(NodeID start,
                                           const std::vector<NodeID> &targets,
                                           CostMetric metric) {
  (void)metric;
  size_t n = m_graph.nodeCount();
  std::vector<float> distF(n, std::numeric_limits<float>::infinity());
  std::priority_queue<SearchState, std::vector<SearchState>, std::greater<>>
      pqF;

  distF[start] = 0.0f;
  pqF.push({0.0f, start});

  std::vector<NodeID> visitedF;
  while (!pqF.empty()) {
    auto [d, u] = pqF.top();
    pqF.pop();
    if (d > distF[u])
      continue;
    visitedF.push_back(u);
    for (const auto &e : m_graph.forwardNeighbors(u)) {
      if (d + e.weight < distF[e.target]) {
        distF[e.target] = d + e.weight;
        pqF.push({distF[e.target], e.target});
      }
    }
  }

  std::vector<float> result(targets.size(),
                            std::numeric_limits<float>::infinity());
  for (size_t i = 0; i < targets.size(); ++i) {
    NodeID target = targets[i];
    if (target >= n)
      continue;

    // Backward search from target
    std::vector<float> distB(n, std::numeric_limits<float>::infinity());
    std::priority_queue<SearchState, std::vector<SearchState>, std::greater<>>
        pqB;
    distB[target] = 0.0f;
    pqB.push({0.0f, target});

    float bestDist = std::numeric_limits<float>::infinity();
    while (!pqB.empty()) {
      auto [d, u] = pqB.top();
      pqB.pop();
      if (d > bestDist)
        break;
      if (d > distB[u])
        continue;

      if (distF[u] != std::numeric_limits<float>::infinity()) {
        bestDist = std::min(bestDist, d + distF[u]);
      }

      for (const auto &e : m_graph.backwardNeighbors(u)) {
        if (d + e.weight < distB[e.target]) {
          distB[e.target] = d + e.weight;
          pqB.push({distB[e.target], e.target});
        }
      }
    }
    result[i] = bestDist;
  }

  return result;
}

} // namespace atr
