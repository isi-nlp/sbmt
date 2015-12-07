cmake_minimum_required(VERSION 2.8)
project(xrsdb_apps)

find_package(Boost REQUIRED)
find_package(TBB REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS})

link_libraries(xrsdb)
include_directories(..)

file(GLOB XSUSRCFILES search/*.cpp)
add_library(xrsdb_search_util ${XSUSRCFILES})
target_include_directories(xrsdb_search_util PUBLIC .)
target_link_libraries(xrsdb_search_util source_syntax cluster_model neural_collins collins nntm srl)

add_executable(readforest readforest.cpp)

add_executable(xrsdb_dump_index dump_index.cpp)

add_executable(xrsdb_dump dump.cpp)

add_executable(xrsdb_batch_retrieval batch_retrieval.cpp)

add_executable(xrsdb_filter filter.cpp)

add_executable(xrsdb_mergetables mergetables.cpp)

add_executable(xrsdb_create create.cpp)

add_executable(xrsdb_unknown_filter unknown_filter.cpp)

add_executable(xrsdb_genfreq genfreq.cpp)

add_executable(xrsdb_makefreq makefreq.cpp)

add_executable(xrsdb_index index.cpp)

add_executable(xrsdb_populate populate.cpp)

add_executable(xrsdb_populate2 populate2.cpp)
target_link_libraries(xrsdb_populate2 xrsdb_search_util)

add_executable(xrsdb_search search.cpp)
target_include_directories(xrsdb_search PRIVATE .)
target_link_libraries(xrsdb_search xrsdb_search_util sbmt_util ${TBB_LIBRARIES} tbb_alloc)

install(TARGETS readforest
                xrsdb_dump_index
                xrsdb_dump
                xrsdb_batch_retrieval
                xrsdb_filter
                xrsdb_create
                xrsdb_unknown_filter
                xrsdb_genfreq
                xrsdb_makefreq
                xrsdb_index
                xrsdb_populate
                xrsdb_populate2
                xrsdb_search
                xrsdb_mergetables
        DESTINATION bin)

install(TARGETS xrsdb_search_util DESTINATION lib)
