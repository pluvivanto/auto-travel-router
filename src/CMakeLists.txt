add_library(atr_impl
    graph_impl.cpp
    dijkstra_router.cpp
    osm_reader.cpp
    server.cpp
    optimizer.cpp
    ch_graph.cpp
    ch_preprocessor.cpp
    ch_router.cpp
)
target_link_libraries(atr_impl PUBLIC atr_core Boost::context Boost::coroutine)

add_executable(atr_server main.cpp)
target_link_libraries(atr_server PRIVATE atr_impl)
