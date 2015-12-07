cmake_minimum_required(VERSION 2.8)
project(rule_length)

find_package(Boost REQUIRED COMPONENTS program_options coroutine)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(.)
add_library(rule_length_info rule_length_info.cpp)
target_link_libraries(rule_length_info sbmt)
target_include_directories(rule_length_info INTERFACE .)

add_executable(rule_length_mapper rule_length_mapper.cpp)
target_link_libraries(rule_length_mapper xrsparse ${Boost_LIBRARIES})

install(TARGETS rule_length_mapper DESTINATION bin)
install(TARGETS rule_length_info DESTINATION lib)
install(PROGRAMS rule_length_reducer DESTINATION bin)

