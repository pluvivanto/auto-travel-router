#include "atr/ch_router.hpp"
#include <algorithm>
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

using MinHeap = std::priority_queue<SearchState, std::vector<SearchState>,
                                    std::greater<>>;

std::optional<RouteResult> CHRouter::findRoute(NodeID start, NodeID end,
                                               CostMetric metric) {
  (void)metric;
  if (start == end)
    return RouteResult{{start}, 0.0f};

  size_t n = m_graph.nodeCount();
  std::vector<float> distF(n, INF_DIST);
  std::vector<float> distB(n, INF_DIST);
  std::vector<EdgeInfo> parentF(n, {NO_NODE,
                                    NO_NODE, true});
  std::vector<EdgeInfo> parentB(n, {NO_NODE,
                                    NO_NODE, false});

  MinHeap pqF, pqB;

  distF[start] = 0.0f;
  pqF.push({0.0f, start});
  distB[end] = 0.0f;
  pqB.push({0.0f, end});

  float bestDist = INF_DIST;
  NodeID meetingNode = NO_NODE;

  while (!pqF.empty() || !pqB.empty()) {
    if (!pqF.empty()) {
      auto [d, u] = pqF.top();
      pqF.pop();
      if (d >= bestDist) {
        pqF = {}; // Clear queue to stop forward search
      } else if (d <= distF[u]) {
        for (const auto &e : m_graph.forwardNeighbors(u)) {
          float newDist = d + e.weight;
          if (newDist < distF[e.target]) {
            distF[e.target] = newDist;
            parentF[e.target] = {u, e.skippedNode, true};
            pqF.push({newDist, e.target});
            if (distB[e.target] != INF_DIST) {
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
      if (d >= bestDist) {
        pqB = {}; // Clear queue to stop backward search
      } else if (d <= distB[u]) {
        for (const auto &e : m_graph.backwardNeighbors(u)) {
          float newDist = d + e.weight;
          if (newDist < distB[e.target]) {
            distB[e.target] = newDist;
            parentB[e.target] = {u, e.skippedNode, false};
            pqB.push({newDist, e.target});
            if (distF[e.target] != INF_DIST) {
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

  if (bestDist == INF_DIST)
    return std::nullopt;

  std::vector<NodeID> path;

  auto findSkipped = [&](NodeID from, NodeID to) -> NodeID {
    if (m_graph.level(to) > m_graph.level(from)) {
      for (auto &e : m_graph.forwardNeighbors(from))
        if (e.target == to)
          return e.skippedNode;
    } else {
      for (auto &e : m_graph.backwardNeighbors(to))
        if (e.target == from)
          return e.skippedNode;
    }
    return NO_NODE;
  };

  auto unpack = [&](auto self, NodeID u, NodeID v, NodeID skipped) -> void {
    if (skipped == NO_NODE) {
      path.push_back(v);
    } else {
      self(self, u, skipped, findSkipped(u, skipped));
      self(self, skipped, v, findSkipped(skipped, v));
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
  std::vector<float> distF(n, INF_DIST);
  MinHeap pqF;

  distF[start] = 0.0f;
  pqF.push({0.0f, start});

  while (!pqF.empty()) {
    auto [d, u] = pqF.top();
    pqF.pop();
    if (d > distF[u])
      continue;
    for (const auto &e : m_graph.forwardNeighbors(u)) {
      if (d + e.weight < distF[e.target]) {
        distF[e.target] = d + e.weight;
        pqF.push({distF[e.target], e.target});
      }
    }
  }

  std::vector<float> result(targets.size(), INF_DIST);
  std::vector<float> distB(n, INF_DIST);
  std::vector<NodeID> touched;

  for (size_t i = 0; i < targets.size(); ++i) {
    NodeID target = targets[i];
    if (target >= n)
      continue;

    distB[target] = 0.0f;
    touched.push_back(target);
    MinHeap pqB;
    pqB.push({0.0f, target});

    float bestDist = INF_DIST;
    while (!pqB.empty()) {
      auto [d, u] = pqB.top();
      pqB.pop();
      if (d > bestDist)
        break;
      if (d > distB[u])
        continue;

      if (distF[u] != INF_DIST)
        bestDist = std::min(bestDist, d + distF[u]);

      for (const auto &e : m_graph.backwardNeighbors(u)) {
        if (d + e.weight < distB[e.target]) {
          distB[e.target] = d + e.weight;
          touched.push_back(e.target);
          pqB.push({distB[e.target], e.target});
        }
      }
    }
    result[i] = bestDist;

    for (NodeID node : touched)
      distB[node] = INF_DIST;
    touched.clear();
  }

  return result;
}

} // namespace atr
