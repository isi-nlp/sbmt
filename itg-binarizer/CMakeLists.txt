cmake_minimum_required(VERSION 2.8)

project(itg_binarizer)

find_package(Boost REQUIRED COMPONENTS system filesystem regex program_options)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(itg_binarizer binal.cc)
target_link_libraries(itg_binarizer ${Boost_LIBRARIES} graehl sbmt)

install(TARGETS itg_binarizer DESTINATION bin)