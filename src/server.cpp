#include "atr/server.hpp"
#include <chrono>
#include <oatpp/core/macro/codegen.hpp>
#include <oatpp/core/macro/component.hpp>
#include <oatpp/network/Server.hpp>
#include <oatpp/network/tcp/server/ConnectionProvider.hpp>
#include <oatpp/parser/json/mapping/ObjectMapper.hpp>
#include <oatpp/web/protocol/http/outgoing/ResponseFactory.hpp>
#include <oatpp/web/server/HttpConnectionHandler.hpp>
#include <oatpp/web/server/HttpRouter.hpp>
#include <oatpp/web/server/api/ApiController.hpp>

namespace atr {

#include OATPP_CODEGEN_BEGIN(DTO)
class PointDto : public oatpp::DTO {
  DTO_INIT(PointDto, DTO)
  DTO_FIELD(Float64, lat);
  DTO_FIELD(Float64, lon);
};
class NodeDto : public oatpp::DTO {
  DTO_INIT(NodeDto, DTO)
  DTO_FIELD(UInt32, nodeId);
  DTO_FIELD(Float64, lat);
  DTO_FIELD(Float64, lon);
};
class DailyItineraryDto : public oatpp::DTO {
  DTO_INIT(DailyItineraryDto, DTO)
  DTO_FIELD(Vector<Object<NodeDto>>, sequence);
  DTO_FIELD(Vector<Object<NodeDto>>, fullPath);
};
class OptimizeResponseDto : public oatpp::DTO {
  DTO_INIT(OptimizeResponseDto, DTO)
  DTO_FIELD(String, status);
  DTO_FIELD(Vector<Object<DailyItineraryDto>>, days);
};
class OptimizeRequestDto : public oatpp::DTO {
  DTO_INIT(OptimizeRequestDto, DTO)
  DTO_FIELD(Object<PointDto>, hotel);
  DTO_FIELD(Int32, numDays);
  DTO_FIELD(Vector<Object<PointDto>>, pois);
};
#include OATPP_CODEGEN_END(DTO)

#include OATPP_CODEGEN_BEGIN(ApiController)
class RouterController : public oatpp::web::server::api::ApiController {
public:
  RouterController(const std::shared_ptr<ObjectMapper> &m,
                   std::shared_ptr<Graph> g, std::shared_ptr<Router> r,
                   std::shared_ptr<Optimizer> o)
      : ApiController(m), m_g(g), m_r(r), m_o(o) {}

  static std::shared_ptr<RouterController>
  createShared(const std::shared_ptr<ObjectMapper> &m, std::shared_ptr<Graph> g,
               std::shared_ptr<Router> r, std::shared_ptr<Optimizer> o) {
    return std::make_shared<RouterController>(m, g, r, o);
  }

  ENDPOINT("POST", "/optimize", optimize,
           BODY_DTO(Object<OptimizeRequestDto>, b)) {
    auto t1 = std::chrono::high_resolution_clock::now();
    std::vector<POI> pois;
    if (b->pois)
      for (const auto &p : *b->pois)
        pois.emplace_back(m_g->findNearestNode(p->lat, p->lon), p->lat,
                          p->lon);

    auto res =
        m_o->optimize(m_g->findNearestNode(b->hotel->lat, b->hotel->lon),
                      pois, b->numDays);
    auto responseDto = OptimizeResponseDto::createShared();
    responseDto->status = "success";
    responseDto->days =
        oatpp::Vector<oatpp::Object<DailyItineraryDto>>::createShared();

    for (const auto &day : res.days) {
      auto dayDto = DailyItineraryDto::createShared();
      dayDto->sequence = oatpp::Vector<oatpp::Object<NodeDto>>::createShared();
      dayDto->fullPath =
          oatpp::Vector<oatpp::Object<NodeDto>>::createShared();
      for (auto nid : day.sequence) {
        const auto &n = m_g->nodeDetails(nid);
        auto d = NodeDto::createShared();
        d->nodeId = nid;
        d->lat = n.lat;
        d->lon = n.lon;
        dayDto->sequence->push_back(d);
      }
      for (auto nid : day.fullPath) {
        const auto &n = m_g->nodeDetails(nid);
        auto d = NodeDto::createShared();
        d->nodeId = nid;
        d->lat = n.lat;
        d->lon = n.lon;
        dayDto->fullPath->push_back(d);
      }
      responseDto->days->push_back(dayDto);
    }
    auto response = createDtoResponse(Status::CODE_200, responseDto);
    auto dur =
        std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(
            std::chrono::high_resolution_clock::now() - t1);
    response->putHeader("X-Response-Time-Ms", std::to_string(dur.count()));
    return response;
  }

private:
  std::shared_ptr<Graph> m_g;
  std::shared_ptr<Router> m_r;
  std::shared_ptr<Optimizer> m_o;
};
#include OATPP_CODEGEN_END(ApiController)

Server::Server(std::unique_ptr<Graph> g, std::unique_ptr<Router> r,
               std::unique_ptr<Optimizer> o)
    : m_graph(std::move(g)), m_router(std::move(r)), m_optimizer(std::move(o)) {}

void Server::run(ServerConfig config) {
  oatpp::base::Environment::init();
  auto router = oatpp::web::server::HttpRouter::createShared();
  auto objectMapper =
      oatpp::parser::json::mapping::ObjectMapper::createShared();
  router->addController(RouterController::createShared(objectMapper, std::move(m_graph), std::move(m_router), std::move(m_optimizer)));
  oatpp::network::Server(
      oatpp::network::tcp::server::ConnectionProvider::createShared(
          {config.address, config.port, oatpp::network::Address::IP_4}),
      oatpp::web::server::HttpConnectionHandler::createShared(router))
      .run();
  oatpp::base::Environment::destroy();
}
} // namespace atr
