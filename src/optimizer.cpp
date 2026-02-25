#include "atr/optimizer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <oatpp/core/base/Environment.hpp>

namespace atr {
using namespace std::chrono;

Optimizer::Optimizer(Graph &g, Router &r)
    : m_router(r) { (void)g; }

std::vector<Optimizer::Cluster>
Optimizer::clusterPois(const std::vector<POI> &pois, int k) {
  if (pois.empty() || k <= 0) return {};
  if (pois.size() <= (size_t)k) {
    std::vector<Cluster> res;
    for (auto &p : pois) res.push_back({{p}, p.lat, p.lon});
    return res;
  }

  std::vector<Cluster> clusters(k);
  for (int i = 0; i < k; ++i) {
    clusters[i].centerLat = pois[i].lat;
    clusters[i].centerLon = pois[i].lon;
  }

  for (int iter = 0; iter < 20; ++iter) {
    for (auto &c : clusters) c.pois.clear();
    for (auto &p : pois) {
      auto it = std::min_element(clusters.begin(), clusters.end(),
                                 [&](auto &a, auto &b) {
                                   return (std::pow(p.lat - a.centerLat, 2) +
                                           std::pow(p.lon - a.centerLon, 2)) <
                                          (std::pow(p.lat - b.centerLat, 2) +
                                           std::pow(p.lon - b.centerLon, 2));
                                 });
      it->pois.push_back(p);
    }
    bool changed = false;
    for (auto &c : clusters) {
      if (c.pois.empty()) continue;
      double lt = 0, ln = 0;
      for (auto &p : c.pois) { lt += p.lat; ln += p.lon; }
      lt /= c.pois.size(); ln /= c.pois.size();
      if (std::abs(lt - c.centerLat) > 1e-6) {
        c.centerLat = lt; c.centerLon = ln; changed = true;
      }
    }
    if (!changed) break;
  }
  return clusters;
}

DailyItinerary Optimizer::optimizeDay(NodeID hotel,
                                       const std::vector<POI> &dayPois,
                                       CostMetric metric) {
  if (dayPois.empty()) return {{hotel}, {hotel}};
  std::vector<NodeID> nodes = {hotel};
  for (auto &p : dayPois) nodes.push_back(p.nodeId);

  size_t n = nodes.size();
  std::vector<std::vector<float>> mat(n, std::vector<float>(n, 1e9f));
  for (size_t i = 0; i < n; ++i)
    for (size_t j = 0; j < n; ++j)
      if (i != j) if (auto r = m_router.findRoute(nodes[i], nodes[j], metric)) mat[i][j] = r->totalCost;

  std::vector<size_t> path = {0};
  std::vector<bool> vis(n, false); vis[0] = true;
  for (size_t i = 1; i < n; ++i) {
    size_t best = 0; float minC = 1e9f;
    for (size_t j = 0; j < n; ++j)
      if (!vis[j] && mat[path.back()][j] < minC) { minC = mat[path.back()][j]; best = j; }
    path.push_back(best); vis[best] = true;
  }
  path.push_back(0);

  for (bool improved = true; improved;) {
    improved = false;
    for (size_t i = 1; i < path.size() - 2; ++i)
      for (size_t j = i + 1; j < path.size() - 1; ++j)
        if (mat[path[i - 1]][path[j]] + mat[path[i]][path[j + 1]] <
            mat[path[i - 1]][path[i]] + mat[path[j]][path[j + 1]]) {
          std::reverse(path.begin() + i, path.begin() + j + 1); improved = true;
        }
  }

  DailyItinerary di;
  for (size_t i = 0; i < path.size(); ++i) {
    NodeID currentNode = nodes[path[i]];
    di.sequence.push_back(currentNode);
    if (i > 0) {
      if (auto r = m_router.findRoute(nodes[path[i - 1]], currentNode, metric)) {
        if (!di.fullPath.empty()) di.fullPath.pop_back();
        di.fullPath.insert(di.fullPath.end(), r->path.begin(), r->path.end());
      } else di.fullPath.push_back(currentNode);
    } else di.fullPath.push_back(currentNode);
  }
  return di;
}

OptimizationResult Optimizer::optimize(NodeID hotel,
                                       const std::vector<POI> &pois,
                                       int numDays, CostMetric metric) {
  auto t1 = high_resolution_clock::now();
  auto clusters = clusterPois(pois, numDays);
  OptimizationResult res;
  for (auto &c : clusters) res.days.push_back(optimizeDay(hotel, c.pois, metric));
  OATPP_LOGI("Optimizer", "Job finished in %lld ms", (long long)duration_cast<milliseconds>(high_resolution_clock::now() - t1).count());
  return res;
}
} // namespace atr
