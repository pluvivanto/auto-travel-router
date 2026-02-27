#include "atr/ch_preprocessor.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

namespace atr {

struct TempCHEdge {
  NodeID target;
  float weight;
  NodeID skipped;
};

struct DynamicGraph {
  std::vector<std::vector<TempCHEdge>> forward;
  std::vector<std::vector<TempCHEdge>> backward;
  std::vector<bool> contracted;

  DynamicGraph(size_t n) : forward(n), backward(n), contracted(n, false) {}

  void addEdge(NodeID u, NodeID v, float weight,
               NodeID skipped = std::numeric_limits<NodeID>::max()) {
    forward[u].push_back({v, weight, skipped});
    backward[v].push_back({u, weight, skipped});
  }

  void updateEdge(NodeID u, NodeID v, float weight, NodeID skipped) {
    for (auto &e : forward[u]) {
      if (e.target == v) {
        if (weight < e.weight) {
          e.weight = weight;
          e.skipped = skipped;
        }
        goto found_forward;
      }
    }
    forward[u].push_back({v, weight, skipped});
  found_forward:
    for (auto &e : backward[v]) {
      if (e.target == u) {
        if (weight < e.weight) {
          e.weight = weight;
          e.skipped = skipped;
        }
        return;
      }
    }
    backward[v].push_back({u, weight, skipped});
  }
};

float witnessSearch(const DynamicGraph &dg, NodeID start, NodeID end,
                    NodeID avoid, float limit) {
  using State = std::pair<float, NodeID>;
  std::priority_queue<State, std::vector<State>, std::greater<State>> pq;
  std::unordered_map<NodeID, float> dist;

  pq.push({0.0f, start});
  dist[start] = 0.0f;

  int settledCount = 0;
  const int maxSettled = 64; // Limit search space for speed

  while (!pq.empty()) {
    auto [d, u] = pq.top();
    pq.pop();

    if (d > limit)
      return std::numeric_limits<float>::infinity();
    if (u == end)
      return d;
    if (d > dist[u])
      continue;

    if (++settledCount > maxSettled)
      break;

    for (const auto &e : dg.forward[u]) {
      if (e.target == avoid || dg.contracted[e.target])
        continue;
      float newDist = d + e.weight;
      if (newDist > limit)
        continue;
      if (dist.find(e.target) == dist.end() || newDist < dist[e.target]) {
        dist[e.target] = newDist;
        pq.push({newDist, e.target});
      }
    }
  }
  return std::numeric_limits<float>::infinity();
}

std::unique_ptr<CHGraph> CHPreprocessor::preprocess(const StaticGraph &graph,
                                                    CostMetric metric) {
  size_t n = graph.nodeCount();
  DynamicGraph dg(n);

  for (size_t u = 0; u < n; ++u) {
    for (const auto &e : graph.neighbors(static_cast<NodeID>(u))) {
      float weight = (metric == CostMetric::Distance) ? e.distance : e.duration;
      dg.addEdge(static_cast<NodeID>(u), e.target, weight);
    }
  }

  std::vector<uint32_t> levels(n, 0);
  auto calcImportance = [&](NodeID u) {
    int shortcuts = 0;
    int incidentEdges = 0;
    for (auto &in : dg.backward[u]) {
      if (dg.contracted[in.target])
        continue;
      incidentEdges++;
      for (auto &out : dg.forward[u]) {
        if (dg.contracted[out.target])
          continue;
        if (in.target == out.target)
          continue;
        shortcuts++;
      }
    }
    return shortcuts - incidentEdges;
  };

  using NodePriority = std::pair<int, NodeID>;
  std::priority_queue<NodePriority, std::vector<NodePriority>,
                      std::greater<NodePriority>>
      pq;

  for (size_t i = 0; i < n; ++i) {
    pq.push({calcImportance(static_cast<NodeID>(i)), static_cast<NodeID>(i)});
  }

  uint32_t currentLevel = 0;
  int contractedCount = 0;

  while (!pq.empty()) {
    auto [importance, v] = pq.top();
    pq.pop();

    if (dg.contracted[v])
      continue;

    // Lazy re-evaluation
    int currentImp = calcImportance(v);
    if (!pq.empty() && currentImp > pq.top().first) {
      pq.push({currentImp, v});
      continue;
    }

    levels[v] = currentLevel++;
    dg.contracted[v] = true;

    if (++contractedCount % 10000 == 0) {
      std::cout << "Contracted " << contractedCount << " / " << n << " nodes"
                << std::endl;
    }

    for (const auto &in : dg.backward[v]) {
      if (dg.contracted[in.target])
        continue;
      for (const auto &out : dg.forward[v]) {
        if (dg.contracted[out.target])
          continue;
        if (in.target == out.target)
          continue;

        float weight = in.weight + out.weight;
        float wdist = witnessSearch(dg, in.target, out.target, v, weight);
        if (wdist > weight) {
          dg.updateEdge(in.target, out.target, weight, v);
        }
      }
    }
  }

  std::vector<CHEdge> finalForward;
  std::vector<size_t> forwardOffsets(n + 1, 0);
  std::vector<CHEdge> finalBackward;
  std::vector<size_t> backwardOffsets(n + 1, 0);

  for (size_t u = 0; u < n; ++u) {
    for (const auto &e : dg.forward[u]) {
      if (levels[e.target] > levels[u]) {
        finalForward.push_back({e.target, e.weight, e.skipped});
        forwardOffsets[u + 1]++;
      }
    }
    for (const auto &e : dg.backward[u]) {
      if (levels[e.target] > levels[u]) {
        finalBackward.push_back({e.target, e.weight, e.skipped});
        backwardOffsets[u + 1]++;
      }
    }
  }

  for (size_t i = 0; i < n; ++i) {
    forwardOffsets[i + 1] += forwardOffsets[i];
    backwardOffsets[i + 1] += backwardOffsets[i];
  }

  std::vector<CHEdge> sortedForward(finalForward.size());
  std::vector<size_t> curF = forwardOffsets;
  for (size_t u = 0; u < n; ++u) {
    for (const auto &e : dg.forward[u]) {
      if (levels[e.target] > levels[u]) {
        sortedForward[curF[u]++] = {e.target, e.weight, e.skipped};
      }
    }
  }

  std::vector<CHEdge> sortedBackward(finalBackward.size());
  std::vector<size_t> curB = backwardOffsets;
  for (size_t u = 0; u < n; ++u) {
    for (const auto &e : dg.backward[u]) {
      if (levels[e.target] > levels[u]) {
        sortedBackward[curB[u]++] = {e.target, e.weight, e.skipped};
      }
    }
  }

  std::vector<Node> nodes(n);
  for (size_t i = 0; i < n; ++i)
    nodes[i] = graph.nodeDetails(static_cast<NodeID>(i));

  return std::make_unique<CHGraph>(
      std::move(nodes), std::move(sortedForward), std::move(forwardOffsets),
      std::move(sortedBackward), std::move(backwardOffsets), std::move(levels));
}

} // namespace atr
