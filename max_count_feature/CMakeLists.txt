cmake_minimum_required(VERSION 2.8)
project(max_count_feature)

find_package(Boost REQUIRED COMPONENTS program_options regex thread system locale)
include_directories(${Boost_INCLUDE_DIRS} .)

add_executable(rootcount rootcount.cpp)
target_link_libraries(rootcount xrsparse)

add_executable(root_hw_count root_hw_count.cpp)
target_link_libraries(root_hw_count xrsparse ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(rootprob rootprob.cpp)
target_link_libraries(rootprob xrsparse)

add_executable(rootprob_reducer rootprob_reducer.cpp)
target_link_libraries(rootprob_reducer xrsparse)

add_executable(taglex taglex.cpp)
target_link_libraries(taglex sbmt gusc)

add_executable(root_hw_prob root_hw_prob.cpp)
target_link_libraries(root_hw_prob xrsparse ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(remove_internals remove_internals.cpp)
target_link_libraries(remove_internals xrsparse)

add_executable(splitfilter splitfilter.cpp)
target_link_libraries(splitfilter xrsparse)

add_executable(splitmarker splitmarker.cpp)
target_link_libraries(splitmarker xrsparse)

add_executable(max_feature_mapper max_feature_mapper.cpp)
target_link_libraries(max_feature_mapper xrsparse)

add_executable(fields fields.cpp)
target_link_libraries(fields gusc)

add_executable(rule_all_features_mapper rule_all_features_mapper.cpp)
target_link_libraries(rule_all_features_mapper xrsparse)

add_executable(max_feature_reducer max_feature_reducer.cpp)

add_executable(featval_mapper featval_mapper.cpp)
target_link_libraries(featval_mapper xrsparse)

add_executable(rule_target_words rule_target_words.cpp)
target_link_libraries(rule_target_words xrsparse)

add_executable(shuffle shuffle.cpp)

add_executable(fliprule fliprule.cpp)
target_link_libraries(fliprule sbmt xrsparse)

add_executable(rule_id_map rule_id_map.cpp)
target_link_libraries(rule_id_map xrsparse)

add_executable(count_feature_mapper count_feature_mapper.cpp)
target_link_libraries(count_feature_mapper xrsparse)

add_executable(collapse_mapper collapse_mapper.cpp)
target_link_libraries(collapse_mapper xrsparse)

add_executable(idfirst idfirst.cpp)

add_executable(emitdict emitdict.cpp)
target_link_libraries(emitdict xrsparse ${Boost_REGEX_LIBRARY})

add_executable(ifea ifea.cpp)
target_link_libraries(ifea xrsparse sbmt)

add_executable(ratio_mapper ratio_mapper.cpp)
target_link_libraries(ratio_mapper xrsparse)

add_executable(ratio_reducer ratio_reducer.cpp)

add_executable(nbcat nbcat.cpp)
target_link_libraries(nbcat ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY})

add_executable(rule_training_length_mapper rule_training_length_mapper.cpp)
target_link_libraries(rule_training_length_mapper xrsparse)

add_executable(lex_mapper1 lex_mapper1.cpp)
target_link_libraries(lex_mapper1 xrsparse sbmt)

add_executable(ruleviz ruleviz.cpp)
target_link_libraries(ruleviz xrsparse sbmt)

add_executable(substrsearch substrsearch.cpp)
target_link_libraries(substrsearch gusc ${Boost_REGEX_LIBRARY})

add_executable(lower lower.cpp)
target_link_libraries(lower ${Boost_LOCALE_LIBRARY})
install(TARGETS remove_internals
                rootcount
                root_hw_count
                rootprob
                rootprob_reducer
                root_hw_prob
                splitfilter
                splitmarker
                shuffle
                emitdict
                rule_all_features_mapper
                max_feature_mapper
                max_feature_reducer
                count_feature_mapper
                collapse_mapper
                rule_id_map
                ratio_mapper
                ratio_reducer
                lex_mapper1
                nbcat
                ifea
                fields
                featval_mapper
                rule_target_words
                fliprule
                idfirst
                rule_training_length_mapper
                ruleviz
                substrsearch
                taglex
                lower
        DESTINATION bin
       )
install(PROGRAMS 
              max_feature_reducer.py
              count_feature_reducer
              count_align_tagged
              count_align_weighted
              count_align_trivial_weighted
              divide
              mkdata
              lex_mapper2
              rule_join_reducer
              rule_join_reducer2
              lc
              collapse_reducer
              ratio_reducer.py
              set_reducer
              rule_training_length_reducer
              reshape_join_mapper.py
              cleanup_rprob_join
        DESTINATION bin
       )
