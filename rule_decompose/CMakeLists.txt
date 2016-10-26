cmake_minimum_required(VERSION 2.8)
project(rule_decompose)

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS} .)
link_libraries(xrsparse sbmt graehl ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_library(binalgo binalgo.cpp)
target_include_directories(binalgo INTERFACE .)

add_executable(leaflm leaflm.cpp)
target_link_libraries(leaflm binalgo)

add_executable(decompose decompose.cpp)
target_link_libraries(decompose binalgo)

add_executable(lhs_size_map lhs_size_map.cpp)
target_link_libraries(lhs_size_map binalgo)

add_executable(justdecomp justdecomp.cpp)
target_link_libraries(justdecomp binalgo)

add_executable(minfilter minfilter.cpp)
target_link_libraries(minfilter xrsparse)

install(TARGETS binalgo DESTINATION lib)
install(TARGETS leaflm
                decompose
                leaflm
                lhs_size_map
                justdecomp
                minfilter
        DESTINATION bin)
install(PROGRAMS train2rule decompbin DESTINATION bin)