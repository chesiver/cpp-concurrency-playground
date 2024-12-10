cmake_minimum_required(VERSION 3.10)

project(template)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")


find_package(fmt CONFIG REQUIRED)
find_package(folly CONFIG REQUIRED)

add_executable(template main.cpp)

target_link_libraries(template PRIVATE fmt::fmt Folly::folly)