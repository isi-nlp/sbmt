cmake_minimum_required(VERSION 2.8)
project(cmph)

add_library( ourcmph 
             src/bdz.c
             src/bdz_ph.c
             src/bitbool.c
             src/bmz8.c
             src/bmz.c
             src/brz.c
             src/buffer_entry.c
             src/buffer_manager.c
             src/chm.c
             src/cmph.c
             src/cmph_structs.c
             src/fch_buckets.c
             src/fch.c
             src/graph.c
             src/hash.c
             src/jenkins_hash.c
             src/vqueue.c
             src/vstack.c
           )
target_include_directories(ourcmph PUBLIC src)           
install(TARGETS ourcmph DESTINATION lib)