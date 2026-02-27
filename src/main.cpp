#include "atr/ch_preprocessor.hpp"
#include "atr/ch_router.hpp"
#include "atr/dijkstra_router.hpp"
#include "atr/optimizer.hpp"
#include "atr/osm_reader.hpp"
#include "atr/server.hpp"
#include <memory>
#include <oatpp/core/base/Environment.hpp>

int main(int argc, char *argv[]) {
  oatpp::base::Environment::init();
  if (argc < 2) {
    OATPP_LOGE("Main", "Usage: %s <path_to.osm.pbf>", argv[0]);
    return 1;
  }

  try {
    OATPP_LOGI("Main", "Loading OSM data...");
    auto staticGraph = atr::OSMReader::readPbf(argv[1]);
    OATPP_LOGI("Main", "Graph loaded: %d nodes, %d edges",
               (int)staticGraph->nodeCount(), (int)staticGraph->edgeCount());

    auto chGraph = atr::CHPreprocessor::preprocess(*staticGraph,
                                                   atr::CostMetric::Duration);
    OATPP_LOGI("Main", "CH Preprocessing finished.");

    auto router = std::make_unique<atr::CHRouter>(*chGraph);
    auto optimizer = std::make_unique<atr::Optimizer>(*chGraph, *router);

    atr::Server server(std::move(chGraph), std::move(router),
                       std::move(optimizer));
    server.run(atr::ServerConfig{});
  } catch (const std::exception &e) {
    OATPP_LOGE("Main", "Fatal error: %s", e.what());
    return 1;
  }

  oatpp::base::Environment::destroy();
  return 0;
}
