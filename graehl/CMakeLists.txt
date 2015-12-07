cmake_minimum_required(VERSION 2.8)

project(graehl)

find_package(Boost REQUIRED COMPONENTS iostreams timer)

include_directories(.. ${Boost_INCLUDE_DIRS})

add_library(graehl shared/gzstream.cpp)
target_link_libraries(graehl z ${Boost_LIBRARIES})
target_include_directories(graehl PUBLIC ..)

install(TARGETS graehl DESTINATION lib)