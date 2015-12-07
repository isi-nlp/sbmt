cmake_minimum_required(VERSION 2.8)
project(Common)

file(GLOB IMPLFILES impl/*.cpp)
file(GLOB SERIALFILES Serializer/*.cpp)
file(GLOB VOCABFILES Vocab/impl/*.cpp)

add_library(LWCommon ${IMPLFILES} ${SERIALFILES} ${VOCABFILES})
target_link_libraries(LWCommon pthread)
target_include_directories(LWCommon PUBLIC . .. ../..)

install(TARGETS LWCommon DESTINATION lib)
