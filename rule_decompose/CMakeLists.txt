cmake_minimum_required(VERSION 2.8)
project(rule_decompose)

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS} .)
link_libraries(xrsparse sbmt graehl ${Boost_PROGRAM_OPTIONS_LIBRARY})
add_library(binalgo binalgo.cpp)
install(TARGETS binalgo DESTINATION lib)