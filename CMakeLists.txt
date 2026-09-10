cmake_minimum_required(VERSION 3.25)

project(MarketReplayEngine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common compiler warnings
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
)

# Functional test executable
add_executable(market_replay_test
    tests/test.cpp
)

target_include_directories( market_replay_test PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Performance benchmark executable
add_executable(market_replay_benchmark
    benchmarks/testing.cpp
)

target_include_directories( market_replay_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

