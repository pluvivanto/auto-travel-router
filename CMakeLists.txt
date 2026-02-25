cmake_minimum_required(VERSION 3.20)
project(auto-travel-router VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include(FetchContent)

find_package(Boost REQUIRED COMPONENTS context coroutine)
find_package(ZLIB REQUIRED)

set(OATPP_BUILD_TESTS OFF CACHE BOOL "Build oatpp internal tests" FORCE)
set(OATPP_LINK_TEST_LIBRARY OFF CACHE BOOL "Link oatpp test library" FORCE)
FetchContent_Declare(
  oatpp
  GIT_REPOSITORY https://github.com/oatpp/oatpp.git
  GIT_TAG 1.3.0-latest
)
FetchContent_MakeAvailable(oatpp)

find_path(OSMIUM_INCLUDE_DIR osmium/visitor.hpp)
find_path(PROTOZERO_INCLUDE_DIR protozero/version.hpp)

add_library(atr_core INTERFACE)
target_include_directories(atr_core INTERFACE include ${OSMIUM_INCLUDE_DIR} ${PROTOZERO_INCLUDE_DIR})
target_link_libraries(atr_core INTERFACE Boost::headers ZLIB::ZLIB oatpp)

add_subdirectory(src)

option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.15.2
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()
