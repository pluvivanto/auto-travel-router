#pragma once

#include "atr/optimizer.hpp"
#include <memory>
#include <string>

namespace atr {

struct ServerConfig {
  std::string address = "0.0.0.0";
  uint16_t port = 8080;
};

class Server {
public:
  Server(std::unique_ptr<Graph> g, std::unique_ptr<Router> r,
         std::unique_ptr<Optimizer> o);
  void run(ServerConfig config = ServerConfig{});

private:
  std::unique_ptr<Graph> m_graph;
  std::unique_ptr<Router> m_router;
  std::unique_ptr<Optimizer> m_optimizer;
};

} // namespace atr
