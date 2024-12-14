cmake_minimum_required(VERSION 3.14)

project(future)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 20)  # Set the desired C++ standard (e.g., 11, 14, 17, 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Make sure the compiler supports the specified standard

include_directories (${CMAKE_SOURCE_DIR}/3rdparty)

find_package(fmt CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(folly CONFIG REQUIRED)

find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
find_path(TASK_THREAD_POOL_INCLUDE_DIRS "task_thread_pool.hpp")

add_executable(future Future/main.cpp)
add_executable(simple_thread_pool Future/simple_thread_pool/simple_thread_pool.cpp)


target_include_directories(future PRIVATE ${BSHOSHANY_THREAD_POOL_INCLUDE_DIRS})
target_include_directories(future PRIVATE ${TASK_THREAD_POOL_INCLUDE_DIRS})

target_link_libraries(future PRIVATE fmt::fmt)
target_link_libraries(future PRIVATE benchmark::benchmark Folly::folly)