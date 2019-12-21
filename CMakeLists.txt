cmake_minimum_required(VERSION 3.15)
project(block_allocator)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O0")

find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})
add_executable(black-test black.hpp test.cpp)
target_link_libraries(black-test GTest::GTest GTest::Main)

add_executable(black-speed-test black.hpp speed_test.cpp)
