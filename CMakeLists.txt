cmake_minimum_required(VERSION 3.24)
project(search_and_data LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(indexing main.cpp)
target_include_directories(indexing PRIVATE .)

add_executable(indexing_tests tests.cpp)
target_include_directories(indexing_tests PRIVATE .)
target_link_libraries(indexing_tests PRIVATE GTest::gtest_main)
enable_testing()
include(GoogleTest)
gtest_discover_tests(indexing_tests)
