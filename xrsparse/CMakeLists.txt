cmake_minimum_required(VERSION 2.8)
project(project_xrsparse)

find_package(Boost REQUIRED COMPONENTS thread)
include_directories(. ${Boost_INCLUDE_DIRS})

add_library(xrsparse src/xrs.cpp)

target_include_directories(xrsparse PUBLIC .)
target_link_libraries(xrsparse ${Boost_LIBRARIES} gusc)

install(TARGETS xrsparse DESTINATION lib)

