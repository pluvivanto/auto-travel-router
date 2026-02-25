#pragma once

#include "atr/static_graph.hpp"
#include <filesystem>
#include <memory>

namespace atr {

struct OSMConfig {
  float defaultSpeed = 50.0f;
};

class OSMReader {
public:
  static std::unique_ptr<StaticGraph>
  readPbf(const std::filesystem::path &path, OSMConfig config = OSMConfig{});
};

} // namespace atr
