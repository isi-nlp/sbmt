cmake_minimum_required(VERSION 2.8)
project(libsbmt)
add_subdirectory(3rdparty/lw)

find_package(Boost REQUIRED COMPONENTS system filesystem thread regex serialization signals iostreams program_options locale)
find_package(TBB REQUIRED)

file(GLOB SRCFILES src/*.cpp)
add_library(sbmt ${SRCFILES})
include_directories(${Boost_INCLUDE_DIRS})
target_include_directories(sbmt PUBLIC include ../boost_sandbox ${TBB_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(sbmt graehl RuleReader gusc biglm LWCommon LWLangModel xrsparse nplm ${Boost_LIBRARIES})
