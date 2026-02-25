#include "atr/osm_reader.hpp"
#include <osmium/geom/haversine.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/io/pbf_input.hpp>
#include <osmium/visitor.hpp>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace atr {

struct WayHandler : public osmium::handler::Handler {
  using LocationIndex =
      osmium::index::map::FlexMem<osmium::unsigned_object_id_type,
                                  osmium::Location>;

  WayHandler(LocationIndex &index, OSMConfig cfg)
      : m_index(index), m_config(cfg) {}

  void way(const osmium::Way &way) {
    const char *highway = way.get_value_by_key("highway");
    if (!highway) return;

    static const std::unordered_set<std::string> validTypes = {
        "motorway", "trunk", "primary", "secondary", "tertiary", "unclassified",
        "residential", "motorway_link", "trunk_link", "primary_link",
        "secondary_link", "tertiary_link"};

    if (validTypes.find(highway) == validTypes.end()) return;

    float speed = parseSpeed(way.get_value_by_key("maxspeed"), highway);
    bool oneWay = isOneWay(way);

    const auto &nodes = way.nodes();
    for (size_t i = 0; i + 1 < nodes.size(); ++i) {
      try {
        auto loc1 = nodes[i].location();
        auto loc2 = nodes[i + 1].location();
        if (!loc1.valid() || !loc2.valid()) continue;

        double dist = osmium::geom::haversine::distance(loc1, loc2);
        float duration = static_cast<float>(dist / (speed / 3.6f));

        NodeID u = getOrCreateNode(nodes[i]);
        NodeID v = getOrCreateNode(nodes[i + 1]);

        m_buildEdges.push_back({u, v, static_cast<float>(dist), duration});
        if (!oneWay) m_buildEdges.push_back({v, u, static_cast<float>(dist), duration});
      } catch (const osmium::not_found &) {}
    }
  }

  float parseSpeed(const char *maxspeed, const char *highway) {
    if (maxspeed) { try { return std::stof(maxspeed); } catch (...) {} }
    std::string h(highway);
    if (h.find("motorway") != std::string::npos) return 130.0f;
    if (h.find("trunk") != std::string::npos) return 100.0f;
    if (h.find("primary") != std::string::npos) return 100.0f;
    if (h.find("secondary") != std::string::npos) return 80.0f;
    return m_config.defaultSpeed;
  }

  bool isOneWay(const osmium::Way &way) {
    const char *ow = way.get_value_by_key("oneway");
    if (ow && std::string(ow) == "yes") return true;
    const char *highway = way.get_value_by_key("highway");
    if (highway && std::string(highway) == "motorway") return true;
    return false;
  }

  NodeID getOrCreateNode(const osmium::NodeRef &nr) {
    auto it = m_osmToInternal.find(nr.ref());
    if (it != m_osmToInternal.end()) return it->second;
    NodeID id = static_cast<NodeID>(m_internalNodes.size());
    m_osmToInternal[nr.ref()] = id;
    m_internalNodes.push_back({nr.lat(), nr.lon(), static_cast<uint64_t>(nr.ref())});
    return id;
  }

  LocationIndex &m_index;
  OSMConfig m_config;
  std::vector<Node> m_internalNodes;
  std::vector<StaticGraph::BuildEdge> m_buildEdges;
  std::unordered_map<osmium::unsigned_object_id_type, NodeID> m_osmToInternal;
};

std::unique_ptr<StaticGraph>
OSMReader::readPbf(const std::filesystem::path &path, OSMConfig config) {
  WayHandler::LocationIndex index;
  osmium::io::Reader reader{path.string(), osmium::io::read_meta::no};
  WayHandler handler{index, config};
  osmium::handler::NodeLocationsForWays<WayHandler::LocationIndex> locationHandler{index};
  osmium::apply(reader, locationHandler, handler);
  reader.close();
  return std::make_unique<StaticGraph>(std::move(handler.m_internalNodes), handler.m_buildEdges);
}

} // namespace atr
