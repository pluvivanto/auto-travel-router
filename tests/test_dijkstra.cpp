#include "atr/dijkstra_router.hpp"
#include "atr/optimizer.hpp"
#include "atr/static_graph.hpp"
#include <gtest/gtest.h>

using namespace atr;

std::unique_ptr<StaticGraph> createTestGraph() {
  std::vector<Node> nodes = {{52.5, 13.4, 1}, {52.6, 13.4, 2}, {52.5, 13.5, 3}};
  std::vector<StaticGraph::BuildEdge> edges = {
      {0, 1, 100.0f, 10.0f}, {1, 2, 200.0f, 20.0f}, {0, 2, 500.0f, 50.0f}};
  return std::make_unique<StaticGraph>(nodes, edges);
}

TEST(GraphTest, NodeCount) {
  auto g = createTestGraph();
  EXPECT_EQ(g->nodeCount(), 3);
  EXPECT_EQ(g->edgeCount(), 3);
}

TEST(RouterTest, DijkstraShortestPath) {
  auto g = createTestGraph();
  DijkstraRouter router(*g);
  auto res = router.findRoute(0, 2, CostMetric::Distance);
  ASSERT_TRUE(res.has_value());
  EXPECT_FLOAT_EQ(res->totalCost, 300.0f);
}

TEST(RouterTest, DijkstraOneToMany) {
  auto g = createTestGraph();
  DijkstraRouter router(*g);
  std::vector<NodeID> targets = {1, 2};
  auto distances = router.findDistances(0, targets, CostMetric::Distance);
  ASSERT_EQ(distances.size(), 2);
  EXPECT_FLOAT_EQ(distances[0], 100.0f);
  EXPECT_FLOAT_EQ(distances[1], 300.0f);
}

TEST(OptimizerTest, Clustering) {
  auto g = createTestGraph();
  auto r = std::make_unique<DijkstraRouter>(*g);
  Optimizer opt(*g, *r);
  std::vector<POI> pois = {{1, 52.6, 13.4}, {2, 52.5, 13.5}};
  auto res = opt.optimize(0, pois, 2);
  EXPECT_EQ(res.days.size(), 2);
}
