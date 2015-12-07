cmake_minimum_required(VERSION 2.8)
project(RuleReader)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_library(RuleReader src/Rule.cc src/RuleNode.cc)
target_include_directories(RuleReader PUBLIC include)
target_link_libraries(RuleReader graehl)
install(TARGETS RuleReader DESTINATION lib)