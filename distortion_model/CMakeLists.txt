cmake_minimum_required(VERSION 2.8)
project(distortion)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
add_library(distortion_model distortion_model.cpp)
target_link_libraries(distortion_model sbmt)
target_include_directories(distortion_model PUBLIC . ${Boost_INCLUDE_DIRS})
install(TARGETS distortion_model DESTINATION lib)
