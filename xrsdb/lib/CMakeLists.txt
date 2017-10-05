cmake_minimum_required(VERSION 2.8)
project(xrsdb_lib)

find_package(Boost REQUIRED COMPONENTS system filesystem regex serialization iostreams program_options)
include_directories(${Boost_INCLUDE_DIRS})

file(GLOB SRCFILES *.cpp)
add_library(xrsdb ${SRCFILES})
target_link_libraries(xrsdb ${Boost_LIBRARIES} gusc graehl sbmt binalgo distortion_model rule_length_info rule_head_info)
target_include_directories(xrsdb PUBLIC . ..)

install(TARGETS xrsdb DESTINATION lib)

