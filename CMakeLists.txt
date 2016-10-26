cmake_minimum_required(VERSION 2.8)
project(sbmt)
string(REPLACE ":" ";" INC_LIST "${CMAKE_INCLUDE_PATH}")
include_directories(${INC_LIST})
string(REPLACE ":" ";" LIB_LIST "${CMAKE_LIBRARY_PATH}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
# add_definitions(-std=c++11)
find_package(Boost REQUIRED)
find_package(TBB REQUIRED)
find_package(EIGEN 3.1.0 REQUIRED)

foreach(lv ${Boost_LIBRARY_DIRS})
  list(APPEND LIB_LIST ${lv})
endforeach(lv)

foreach(lv ${TBB_LIBRARIES})
 get_filename_component(tlv ${lv} DIRECTORY)
 list(APPEND LIB_LIST ${tlv})
endforeach(lv)

list(REMOVE_DUPLICATES LIB_LIST)
link_directories(${LIB_LIST})

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_definitions( -D__GCC3__=1
                 -DLM_NO_COMMON_LIB=1
                 -DLM_NO_QT=1
                 -DSLM_SWAP_BYTES=1
                 -DQT_THREAD_SUPPORT=1
                 -DLM_NO_BDB_LIB=1
                 -DNO_QT=1
                 -DNOTRACE=1
               )

add_subdirectory(NEURAL_LANGUAGE_MODEL)
add_subdirectory(tbb_alloc)
add_subdirectory(gusc)
add_subdirectory(guppy)
add_subdirectory(graehl)
add_subdirectory(xrsparse)
add_subdirectory(ghkm)
add_subdirectory(RuleReader)
add_subdirectory(score)
add_subdirectory(biglm)
add_subdirectory(sbmt_decoder)
add_subdirectory(itg-binarizer)
add_subdirectory(distortion_model)
add_subdirectory(nntm)
add_subdirectory(rule_length)
add_subdirectory(cluster_model)
add_subdirectory(neural_collins)
add_subdirectory(rule_head)
add_subdirectory(max_count_feature)
add_subdirectory(utilities)
add_subdirectory(vocabfilter)
add_subdirectory(rule_decompose)
add_subdirectory(source_syntax)
add_subdirectory(srl)
add_subdirectory(xrsdb)
add_subdirectory(pipelines)
