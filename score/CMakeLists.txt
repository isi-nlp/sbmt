cmake_minimum_required(VERSION 2.8)
project(score)

include_directories(.)
add_executable(scoreTranslation scoreTranslation.cc)
install(TARGETS scoreTranslation DESTINATION bin)
install(PROGRAMS bleu.pl 
                 lw_detokenize.pl 
                 delete-nonlatin.pl 
                 delete-nonlatin.sh 
        DESTINATION bin)
