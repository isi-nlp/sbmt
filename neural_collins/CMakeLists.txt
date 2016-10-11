cmake_minimum_required(VERSION 2.8)
project(neural_collins)

add_library(neural_collins collins.cpp)
target_include_directories(neural_collins PUBLIC ..)
install(TARGETS neural_collins DESTINATION lib)
