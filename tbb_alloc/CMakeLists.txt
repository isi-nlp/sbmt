cmake_minimum_required(VERSION 2.8)
project(tbb_alloc)

find_package(TBB REQUIRED)

add_library(tbb_alloc tbb_alloc.cpp)
target_include_directories(tbb_alloc PUBLIC ${TBB_MALLOC_INCLUDE_DIRS})
target_link_libraries(tbb_alloc ${TBB_MALLOC_LIBRARIES})
