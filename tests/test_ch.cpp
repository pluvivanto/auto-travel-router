#include "atr/ch_preprocessor.hpp"
#include "atr/ch_router.hpp"
#include "atr/dijkstra_router.hpp"
#include "atr/static_graph.hpp"
#include <gtest/gtest.h>

using namespace atr;

std::unique_ptr<StaticGraph> createLinearGraph() {
  std::vector<Node> nodes = {{52.0, 13.0, 1}, {52.1, 13.1, 2}, {52.2, 13.2, 3}};
  std::vector<StaticGraph::BuildEdge> edges = {
      {0, 1, 100.0f, 10.0f}, {1, 2, 200.0f, 20.0f}, {0, 2, 500.0f, 50.0f}};
  return std::make_unique<StaticGraph>(nodes, edges);
}

TEST(CHTest, Preprocessing) {
  auto g = createLinearGraph();
  auto chg = CHPreprocessor::preprocess(*g, CostMetric::Duration);
  EXPECT_EQ(chg->nodeCount(), 3);
}

TEST(CHTest, ShortestPath) {
  auto g = createLinearGraph();
  auto chg = CHPreprocessor::preprocess(*g, CostMetric::Duration);
  CHRouter router(*chg);
  auto res = router.findRoute(0, 2, CostMetric::Duration);
  ASSERT_TRUE(res.has_value());
  EXPECT_FLOAT_EQ(res->totalCost, 30.0f);

  std::vector<NodeID> expected = {0, 1, 2};
  EXPECT_EQ(res->path, expected);
}

TEST(CHTest, OneToMany) {
  auto g = createLinearGraph();
  auto chg = CHPreprocessor::preprocess(*g, CostMetric::Duration);
  CHRouter router(*chg);
  std::vector<NodeID> targets = {1, 2};
  auto distances = router.findDistances(0, targets, CostMetric::Duration);
  ASSERT_EQ(distances.size(), 2);
  EXPECT_FLOAT_EQ(distances[0], 10.0f);
  EXPECT_FLOAT_EQ(distances[1], 30.0f);
}

TEST(CHTest, SameStartAndEnd) {
  auto g = createLinearGraph();
  auto chg = CHPreprocessor::preprocess(*g, CostMetric::Duration);
  CHRouter router(*chg);
  auto res = router.findRoute(0, 0, CostMetric::Duration);
  ASSERT_TRUE(res.has_value());
  EXPECT_FLOAT_EQ(res->totalCost, 0.0f);
  EXPECT_EQ(res->path.size(), 1);
}

TEST(CHTest, Unreachable) {
  std::vector<Node> nodes = {{0, 0, 1}, {1, 1, 2}};
  std::vector<StaticGraph::BuildEdge> edges = {};
  StaticGraph g(nodes, edges);
  auto chg = CHPreprocessor::preprocess(g, CostMetric::Duration);
  CHRouter router(*chg);
  auto res = router.findRoute(0, 1, CostMetric::Duration);
  EXPECT_FALSE(res.has_value());
}

TEST(CHTest, MatchesDijkstra) {
  // Larger graph: verify CH produces same distances as Dijkstra
  std::vector<Node> nodes = {
      {0, 0, 1}, {1, 0, 2}, {0, 1, 3}, {1, 1, 4}, {0.5, 0.5, 5}};
  std::vector<StaticGraph::BuildEdge> edges = {
      {0, 1, 10, 1}, {1, 0, 10, 1}, {0, 2, 20, 2}, {2, 0, 20, 2},
      {1, 3, 20, 2}, {3, 1, 20, 2}, {2, 3, 10, 1}, {3, 2, 10, 1},
      {0, 4, 7, 1},  {4, 0, 7, 1},  {4, 3, 7, 1},  {3, 4, 7, 1}};
  StaticGraph g(nodes, edges);

  DijkstraRouter dijkstra(g);
  auto chg = CHPreprocessor::preprocess(g, CostMetric::Distance);
  CHRouter ch(*chg);

  // Test all pairs
  for (NodeID s = 0; s < 5; ++s) {
    for (NodeID t = 0; t < 5; ++t) {
      auto dRes = dijkstra.findRoute(s, t, CostMetric::Distance);
      auto cRes = ch.findRoute(s, t, CostMetric::Distance);
      if (dRes.has_value()) {
        ASSERT_TRUE(cRes.has_value()) << "s=" << s << " t=" << t;
        EXPECT_FLOAT_EQ(dRes->totalCost, cRes->totalCost)
            << "s=" << s << " t=" << t;
      } else {
        EXPECT_FALSE(cRes.has_value()) << "s=" << s << " t=" << t;
      }
    }
  }
}
