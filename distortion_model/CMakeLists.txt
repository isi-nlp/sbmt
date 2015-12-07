cmake_minimum_required(VERSION 2.8)
project(distortion)

add_library(distortion_model distortion_model.cpp)
target_link_libraries(distortion_model sbmt)
target_include_directories(distortion_model PUBLIC .)
install(TARGETS distortion_model DESTINATION lib)
