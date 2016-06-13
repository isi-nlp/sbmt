cmake_minimum_required(VERSION 2.8)
project(rule_head)
find_package(TBB REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options regex)
include_directories(${Boost_INCLUDE_DIRS} . collins)

### libraries

add_library(rule_head_info rule_head_info2.cpp)
target_include_directories(rule_head_info INTERFACE . collins ${TBB_INCLUDE_DIRS})
target_link_libraries(rule_head_info treelib sbmt ${TBB_LIBRARIES})

add_library(collins collins/lib/collins.cpp)
target_include_directories(collins INTERFACE . collins)
target_link_libraries(collins ${Boost_REGEX_LIBRARY} gusc sbmt treelib)

### executables

add_executable(headvec collins/apps/headvec.cpp)
target_link_libraries(headvec collins gusc sbmt treelib)

add_executable(rule_head_mapper rule_head_mapper.cpp at_replace.cpp)
target_link_libraries(rule_head_mapper xrsparse ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_REGEX_LIBRARY})

add_executable(rule_head_feature_mapper rule_head_feature_mapper.cpp at_replace.cpp)
target_link_libraries(rule_head_feature_mapper xrsparse 
                                               ${Boost_PROGRAM_OPTIONS_LIBRARY} 
                                               ${Boost_REGEX_LIBRARY})

add_executable(nt_head_mapper nt_head_mapper.cpp at_replace.cpp)
target_link_libraries(nt_head_mapper ${Boost_REGEX_LIBRARY} treelib)

add_executable(child_head_mapper child_head_mapper.cpp at_replace.cpp)
target_link_libraries(child_head_mapper ${Boost_REGEX_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} xrsparse)

add_executable(collinsmapper collins/apps/collinsmapper.cpp) 
target_link_libraries(collinsmapper ${Boost_PROGRAM_OPTIONS_LIBRARY} collins)

add_executable(mkcollinslm collins/apps/mkcollinslm.cpp)
target_link_libraries(mkcollinslm collins gusc ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(scoretrees collins/apps/scoretrees.cpp)
target_link_libraries(scoretrees collins treelib)

### install

install(TARGETS rule_head_info collins DESTINATION lib)

install(TARGETS headvec 
                rule_head_mapper 
                rule_head_feature_mapper
                nt_head_mapper 
                child_head_mapper 
                collinsmapper 
                mkcollinslm
                scoretrees
        DESTINATION bin
       )

install(PROGRAMS 
              rule_head_reducer 
              child_head_reducer
              child_head_symbol_mapred
              child_head_symbol_prob_mapper
              rule_head_sum_mapred
              rule_head_join_reducer
              rule_head_nonlex_filter
              rule_head_add_command
              headtables2dict
              collins/apps/mkamrlm.sh
        DESTINATION bin 
       )

