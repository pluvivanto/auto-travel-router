#include "atr/ch_preprocessor.hpp"
#include <iostream>
#include <queue>
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
               NodeID skipped = NO_NODE) {
    forward[u].push_back({v, weight, skipped});
    backward[v].push_back({u, weight, skipped});
  }

  void updateEdge(NodeID u, NodeID v, float weight, NodeID skipped) {
    auto updateOrInsert = [&](std::vector<TempCHEdge> &edges, NodeID tgt) {
      for (auto &e : edges) {
        if (e.target == tgt && weight < e.weight) {
          e.weight = weight;
          e.skipped = skipped;
        }
        if (e.target == tgt)
          return;
      }
      edges.push_back({tgt, weight, skipped});
    };
    updateOrInsert(forward[u], v);
    updateOrInsert(backward[v], u);
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
      return INF_DIST;
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
  return INF_DIST;
}

DynamicGraph buildDynamicGraph(const StaticGraph &graph, CostMetric metric) {
  size_t n = graph.nodeCount();
  DynamicGraph dg(n);
  for (size_t u = 0; u < n; ++u) {
    for (const auto &e : graph.neighbors(static_cast<NodeID>(u))) {
      float weight = (metric == CostMetric::Distance) ? e.distance : e.duration;
      dg.addEdge(static_cast<NodeID>(u), e.target, weight);
    }
  }
  return dg;
}

std::vector<uint32_t> contractNodes(DynamicGraph &dg) {
  size_t n = dg.forward.size();
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

  for (size_t i = 0; i < n; ++i)
    pq.push({calcImportance(static_cast<NodeID>(i)), static_cast<NodeID>(i)});

  uint32_t currentLevel = 0;
  int contractedCount = 0;

  while (!pq.empty()) {
    auto [importance, v] = pq.top();
    pq.pop();

    if (dg.contracted[v])
      continue;

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
        if (wdist > weight)
          dg.updateEdge(in.target, out.target, weight, v);
      }
    }
  }
  return levels;
}

std::unique_ptr<CHGraph> buildCHGraph(const StaticGraph &graph,
                                      const DynamicGraph &dg,
                                      const std::vector<uint32_t> &levels) {
  size_t n = graph.nodeCount();

  std::vector<size_t> forwardOffsets(n + 1, 0);
  std::vector<size_t> backwardOffsets(n + 1, 0);
  for (size_t u = 0; u < n; ++u) {
    for (const auto &e : dg.forward[u])
      if (levels[e.target] > levels[u])
        forwardOffsets[u + 1]++;
    for (const auto &e : dg.backward[u])
      if (levels[e.target] > levels[u])
        backwardOffsets[u + 1]++;
  }
  for (size_t i = 0; i < n; ++i) {
    forwardOffsets[i + 1] += forwardOffsets[i];
    backwardOffsets[i + 1] += backwardOffsets[i];
  }

  std::vector<CHEdge> sortedForward(forwardOffsets[n]);
  std::vector<CHEdge> sortedBackward(backwardOffsets[n]);
  std::vector<size_t> curF = forwardOffsets, curB = backwardOffsets;
  for (size_t u = 0; u < n; ++u) {
    for (const auto &e : dg.forward[u])
      if (levels[e.target] > levels[u])
        sortedForward[curF[u]++] = {e.target, e.weight, e.skipped};
    for (const auto &e : dg.backward[u])
      if (levels[e.target] > levels[u])
        sortedBackward[curB[u]++] = {e.target, e.weight, e.skipped};
  }

  std::vector<Node> nodes(n);
  for (size_t i = 0; i < n; ++i)
    nodes[i] = graph.nodeDetails(static_cast<NodeID>(i));

  return std::make_unique<CHGraph>(
      std::move(nodes), std::move(sortedForward), std::move(forwardOffsets),
      std::move(sortedBackward), std::move(backwardOffsets), std::move(levels));
}

std::unique_ptr<CHGraph> CHPreprocessor::preprocess(const StaticGraph &graph,
                                                    CostMetric metric) {
  auto dg = buildDynamicGraph(graph, metric);
  auto levels = contractNodes(dg);
  return buildCHGraph(graph, dg, levels);
}

} // namespace atr
