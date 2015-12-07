cmake_minimum_required(VERSION 2.8)
project(lwlangmodel)

include_directories(. .. ../.. impl)

file(GLOB IMPLFILES impl/*.cpp)
add_library(LWLangModel ${IMPLFILES})
target_include_directories(LWLangModel INTERFACE ..)
install(TARGETS LWLangModel DESTINATION lib)