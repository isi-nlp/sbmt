cimport std

cdef extern from "biglm.hpp":
    ctypedef int word_type "unsigned int"
    ctypedef struct c_biglm "biglm::lm":
        int order
        word_type lookup_word(std.c_string)
        float lookup_ngram(word_type *, int)
        int get_order()
    cdef c_biglm *new_biglm "new biglm::lm" (std.c_string, bint, double)
    cdef void del_biglm "delete" (c_biglm *)
        
cdef class Ngram:
    cdef c_biglm *ngram
    cdef int order
    cdef float c_lookup_ngram(self, word_type *words, int n)
    cdef word_type c_lookup_word(self, char *s)
    cdef override_unk
    cdef word_type *cache
    cdef int cachesize
    
