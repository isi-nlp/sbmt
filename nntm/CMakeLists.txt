cmake_minimum_required(VERSION 2.8)
project(nntm)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
add_library(nntm nntm_dummy.cpp)
target_link_libraries(nntm sbmt nplm)
target_include_directories(nntm PUBLIC ..)
install(TARGETS nntm DESTINATION lib)


