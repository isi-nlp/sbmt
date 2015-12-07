# biglm.pyx
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

cimport std

cdef extern from "stdlib.h":
   void *malloc(int size)
   void *calloc(int nelem, int size)
   void *realloc(void *buf, int size)
   void free(void *ptr)

NONE = -1 # magic value

cdef class Ngram:
    def __cinit__(self, int order=1, override_unk=None):
        self.ngram = NULL
        self.override_unk=override_unk
	# ignore order
        self.cachesize = 100
        self.cache = <word_type *>malloc(self.cachesize*sizeof(word_type))
        for i from 0 <= i < self.cachesize:
            self.cache[i] = -1

    def __dealloc__(self):
        if self.ngram:
            del_biglm(self.ngram)

    def read(self, filename):
        cdef std.c_string cfilename
        cfilename.assign(filename)
        if self.override_unk is not None:
            self.ngram = new_biglm(cfilename, True, self.override_unk)
        else:
            self.ngram = new_biglm(cfilename, False, 0.0)
        self.order = self.ngram.get_order()

    def lookup_word(self, s):
        cdef std.c_string cs
        cs.assign(s)
        return self.ngram.lookup_word(cs)

    def lookup_ngram(self, words):
        cdef word_type *c_words
        c_words = <word_type *>malloc(len(words)*sizeof(word_type))
        for i from 0 <= i < len(words):
            c_words[i] = words[i]
        return self.ngram.lookup_ngram(c_words, len(words))

    cdef word_type c_lookup_word(self, char *s):
        cdef std.c_string cs
        cs.assign(s)
        return self.ngram.lookup_word(cs)

    cdef float c_lookup_ngram(self, word_type *words, int n):
        return self.ngram.lookup_ngram(words, n)


