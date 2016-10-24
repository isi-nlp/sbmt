cmake_minimum_required(VERSION 2.8)
project(basic)

include_directories(.)

add_library(basic BipartiteGraph.cpp
                  KpartiteGraph.cpp
                  LiBEDebug.cpp
                  MemDgnst.cpp
                  MultierSArray.cpp
                  ScalarEvent.cpp
                  StringVocabulary.cpp
                  SubStringVocabulary.cpp
                  Vocabulary.cpp
                  strmanip.cpp )
                  
target_include_directories(basic PUBLIC .)
install(TARGETS basic DESTINATION lib)