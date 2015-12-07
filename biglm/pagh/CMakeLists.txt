cmake_minimum_required(VERSION 2.8)
project(pagh)

find_package(Boost REQUIRED COMPONENTS iostreams program_options regex)
include_directories(. ${Boost_INCLUDE_DIRS})

add_library(biglm cmph.cpp biglm.cpp quantizer.cpp)
target_include_directories(biglm INTERFACE .)
target_link_libraries(biglm ourcmph ${Boost_IOSTREAMS_LIBRARY})

add_executable(make_biglm make_biglm.cpp arpa.cpp)
target_link_libraries(make_biglm biglm ourcmph ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_REGEX_LIBRARY})

add_executable(use_biglm use_biglm.cpp)
target_link_libraries(use_biglm biglm ourcmph ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_REGEX_LIBRARY})

add_executable(verify_biglm verify_biglm.cpp arpa.cpp)
target_link_libraries(verify_biglm biglm ourcmph ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_REGEX_LIBRARY})

install(TARGETS biglm DESTINATION lib)
install(TARGETS make_biglm use_biglm verify_biglm DESTINATION bin)
