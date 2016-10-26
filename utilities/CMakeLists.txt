cmake_minimum_required(VERSION 2.8)
project(utilities)

find_package(Boost REQUIRED COMPONENTS iostreams thread program_options system filesystem regex)
include_directories(${Boost_INCLUDE_DIRS})

add_library(sbmt_util STATIC lattice_tree_construct.cpp 
                      decode_sequence_reader.cpp 
                      grammar_args.cpp 
                      decoder_filters/decoder_filters.cpp
                      numproc.cpp )
target_include_directories(sbmt_util PUBLIC . decoder_filters)
target_link_libraries(sbmt_util sbmt gusc 
                                ${Boost_SYSTEM_LIBRARY} 
                                ${Boost_FILESYSTEM_LIBRARY})

add_executable(cn_to_lattice apps/cn_to_lattice/cn_to_lattice.cpp)
target_link_libraries(cn_to_lattice gusc)

add_executable(filter_bad_rules apps/filter_bad_rules/filter_bad_rules.cpp)
target_link_libraries(filter_bad_rules sbmt)

add_executable(lattice_words apps/lattice_words/lattice_words.cpp)
target_link_libraries(lattice_words gusc ${Boost_THREAD_LIBRARY})

add_executable(prepend_start_words apps/prepend_start_words/prepend_start_words.cpp)
target_link_libraries(prepend_start_words ${Boost_PROGRAM_OPTIONS_LIBRARY} gusc)

add_executable(validate_lattice apps/validate_lattice/validate_lattice.cpp)
target_link_libraries(validate_lattice gusc sbmt_util)

add_executable(view_runtime_infos apps/view_runtime_infos/view_runtime_infos.cpp)
target_link_libraries(view_runtime_infos sbmt ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(xrs_vocab apps/xrs_vocab/xrs_vocab.cpp)
target_link_libraries(xrs_vocab RuleReader graehl sbmt 
                                ${Boost_REGEX_LIBRARY} 
                                ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(mark_head_xrs decompose_dlm_events/mark_head_xrs.cpp)
target_link_libraries(mark_head_xrs ${Boost_SYSTEM_LIBRARY} 
                                    ${Boost_FILESYSTEM_LIBRARY} 
                                    ${Boost_PROGRAM_OPTIONS_LIBRARY} 
                                    RuleReader 
                                    graehl)

add_executable(new_decoder_weight_format gluetopmaroon/new_decoder_weight_format.cpp
                                         gluetopmaroon/extra_rules.cpp)

target_link_libraries(new_decoder_weight_format graehl sbmt pthread 
                                                ${Boost_PROGRAM_OPTIONS_LIBRARY} 
                                                ${Boost_REGEX_LIBRARY} 
                                                ${Boost_SYSTEM_LIBRARY} 
                                                ${Boost_FILESYSTEM_LIBRARY})

add_executable(glue_rules gluetopmaroon/glue_rules.cpp gluetopmaroon/extra_rules.cpp)
target_link_libraries(glue_rules gusc xrsparse ${Boost_PROGRAM_OPTIONS_LIBRARY} 
                                 ${Boost_SYSTEM_LIBRARY} 
                                 ${Boost_FILESYSTEM_LIBRARY} 
                                 ${Boost_REGEX_LIBRARY})

add_executable(maroon_rules gluetopmaroon/maroon_rules.cpp gluetopmaroon/extra_rules.cpp)
target_link_libraries(maroon_rules gusc xrsparse 
                                   ${Boost_PROGRAM_OPTIONS_LIBRARY}
                                   ${Boost_SYSTEM_LIBRARY} 
                                   ${Boost_FILESYSTEM_LIBRARY} 
                                   ${Boost_REGEX_LIBRARY})

add_executable(splitnt splitnt/splitnt.cpp)
target_link_libraries(splitnt xrsparse)

add_executable(echo_rules echo_rules.cpp)
target_link_libraries(echo_rules sbmt)

add_executable(numcpu numcpu.cpp)
target_link_libraries(numcpu sbmt_util)

add_executable(target_word_features target_word_features/target_word_features.cpp)
target_link_libraries(target_word_features sbmt)

add_executable(unknown_word_rules unknowns/unknown_word_rules.cpp)
target_link_libraries(unknown_word_rules 
                      sbmt graehl xrsparse
                      ${Boost_PROGRAM_OPTIONS_LIBRARY} 
                      ${Boost_SYSTEM_LIBRARY} 
                      ${Boost_FILESYSTEM_LIBRARY})

add_executable(unknown_words unknowns/unknown_words.cpp)
target_link_libraries(unknown_words 
                      sbmt gusc
                      ${Boost_PROGRAM_OPTIONS_LIBRARY} 
                      ${Boost_SYSTEM_LIBRARY} 
                      ${Boost_FILESYSTEM_LIBRARY}
                      ${Boost_IOSTREAMS_LIBRARY})

install(TARGETS cn_to_lattice
                filter_bad_rules
                prepend_start_words
                validate_lattice
                xrs_vocab
                lattice_words
                numcpu
                echo_rules
                splitnt
                new_decoder_weight_format
                glue_rules
                maroon_rules
                target_word_features
                unknown_word_rules
                unknown_words
                view_runtime_infos
        DESTINATION bin)

file(GLOB SCRIPTFILES *.pl *.sh *.py *.pm *.jar)
install(PROGRAMS ${SCRIPTFILES} gluetopmaroon/toprules DESTINATION bin)

                  
