cmake_minimum_required(VERSION 2.8)
project(srl)

add_library(srl agreement/info.cpp)
target_link_libraries(srl sbmt)

install(TARGETS srl DESTINATION lib)
