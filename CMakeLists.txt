set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # to remove warnings coming from dependencies' old cmake policy version
cmake_minimum_required(VERSION 3.20)
project(auto-travel-router VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-poison-system-directories")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(ZLIB REQUIRED)

include(FetchContent)

# Header-only
FetchContent_Declare(
  protozero
  GIT_REPOSITORY https://github.com/mapbox/protozero.git
  GIT_TAG v1.7.1
)
FetchContent_Declare(
  libosmium
  GIT_REPOSITORY https://github.com/osmcode/libosmium.git
  GIT_TAG v2.20.0
)
FetchContent_GetProperties(protozero)
if(NOT protozero_POPULATED)
  FetchContent_Populate(protozero)
endif()
FetchContent_GetProperties(libosmium)
if(NOT libosmium_POPULATED)
  FetchContent_Populate(libosmium)
endif()

set(OATPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(OATPP_LINK_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  oatpp
  GIT_REPOSITORY https://github.com/oatpp/oatpp.git
  GIT_TAG 1.3.0-latest
  SYSTEM
)
FetchContent_MakeAvailable(oatpp)
target_compile_options(oatpp PRIVATE -w)

add_library(atr_core INTERFACE)
target_compile_options(atr_core INTERFACE -Wall -Wextra)
target_include_directories(atr_core INTERFACE include)
target_include_directories(atr_core SYSTEM INTERFACE
    ${protozero_SOURCE_DIR}/include
    ${libosmium_SOURCE_DIR}/include
)
target_link_libraries(atr_core INTERFACE 
    ZLIB::ZLIB
    oatpp
)

add_subdirectory(src)

option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.17.0
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()
