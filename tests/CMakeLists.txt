add_executable(unit_tests
    test_dijkstra.cpp
)

target_link_libraries(unit_tests PRIVATE atr_impl GTest::gtest)

add_test(NAME unit_tests COMMAND unit_tests)
