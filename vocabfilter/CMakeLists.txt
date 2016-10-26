cmake_minimum_required(VERSION 2.8)
project(vocabfilter)

add_executable(vocabfilter vocabfilter.cpp)
target_link_libraries(vocabfilter sbmt xrsparse)
install(TARGETS vocabfilter DESTINATION bin)
