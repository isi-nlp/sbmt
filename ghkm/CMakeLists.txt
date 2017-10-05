cmake_minimum_required(VERSION 2.8)
project(ghkm)

add_subdirectory(basic)

find_package(Boost REQUIRED COMPONENTS regex program_options)
#find_package(BerkeleyDB REQUIRED)
include_directories(. .. nstring xrs-extract/c++-src ${Boost_INCLUDE_DIRS})

add_library(gzstream gzstream/gzstream.C)
target_include_directories(gzstream PUBLIC gzstream)
target_link_libraries(gzstream z)

add_library(treelib treelib/src/treelib/Tree.cpp treelib/src/treelib/TreeNode.cpp)
target_link_libraries(treelib ${Boost_REGEX_LIBRARY})
target_include_directories(treelib PUBLIC treelib/include)

install(TARGETS gzstream treelib DESTINATION lib)

add_executable( fix_collins 
                xrs-extract/c++-src/fix_collins.C 
                xrs-extract/c++-src/TreeNode.C 
                xrs-extract/c++-src/Tree.C 
                xrs-extract/c++-src/MyErr.C )
target_link_libraries(fix_collins gzstream treelib ${Boost_REGEX_LIBRARY})

add_executable( restruct
                restruct/restruct.cpp
                restruct/Restructurer.cpp
                restruct/SyntaxTree.cpp
                restruct/LabelSpan.cpp
                xrs-extract/c++-src/TreeNode.C
                xrs-extract/c++-src/Tree.C
                xrs-extract/c++-src/Alignment.C
                xrs-extract/c++-src/MyErr.C 
              )
target_link_libraries(restruct gzstream treelib basic ${Boost_REGEX_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY})


add_executable( extract
                xrs-extract/c++-src/TreeNode.C 
                xrs-extract/c++-src/Tree.C 
                xrs-extract/c++-src/MyErr.C
                xrs-extract/c++-src/State.C
                xrs-extract/c++-src/RuleInst.C
                xrs-extract/c++-src/RuleNode.C
                xrs-extract/c++-src/RuleDescriptor.C
                xrs-extract/c++-src/RuleRHS.C
                xrs-extract/c++-src/RuleRHS_el.C
                xrs-extract/c++-src/RuleSet.C
                xrs-extract/c++-src/Derivation.C
                xrs-extract/c++-src/DerivationNode.C
                xrs-extract/c++-src/DerivationNodeDescriptor.C
                xrs-extract/c++-src/LabelSpan.C
                xrs-extract/c++-src/Alignment.C
                xrs-extract/c++-src/Variable.C
                xrs-extract/c++-src/ATS.C
                xrs-extract/c++-src/GenDer.C
                xrs-extract/c++-src/WeightF.C
                xrs-extract/c++-src/TreeProjector.C
#                xrs-extract/c++-src/db_access.C
                xrs-extract/c++-src/extract.C )
target_link_libraries(extract 
#                      db 
#                      db_cxx 
                      gzstream treelib ${Boost_REGEX_LIBRARY})

install(TARGETS fix_collins extract restruct DESTINATION bin)
install(PROGRAMS restruct/deint_deriv.pl restruct/deint_deriv_aux.pl DESTINATION bin)
