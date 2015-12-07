cmake_minimum_required(VERSION 2.8)
project(cluster_model)

add_library(cluster_model cluster_info.cpp)
target_include_directories(cluster_model PUBLIC .)
install(TARGETS cluster_model DESTINATION lib)