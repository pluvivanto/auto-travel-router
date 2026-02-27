#include "atr/ch_preprocessor.hpp"
#include "atr/ch_router.hpp"
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
