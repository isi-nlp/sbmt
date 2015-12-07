cmake_minimum_required(VERSION 2.8)
project(source_syntax)

add_library(source_syntax info.cpp)
target_link_libraries(source_syntax sbmt)
target_include_directories(source_syntax PUBLIC .)

install(TARGETS source_syntax DESTINATION lib)