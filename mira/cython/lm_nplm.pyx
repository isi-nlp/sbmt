# distutils: language = c++

import sys, re, math
import model, svector, log
import sym

# lm.pyx
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

# this module should be checked thoroughly

cdef extern from "stdlib.h":
    void *malloc(int size)
    void *calloc(int nelem, int size)
    void *realloc(void *buf, int size)
    void free(void *ptr)
    
cdef extern from "ctype.h":
    int tolower(int c)

import nplm
cimport nplm

import sym
cimport sym

cdef int START, STOP, UNKNOWN, HOLE
START = sym.fromstring("<s>")
STOP = sym.fromstring("</s>")
UNKNOWN = sym.fromstring("<unk>")
HOLE = sym.fromstring("<elided>")

import rule
cimport rule

"""strings with holes
a hole stands for a run of symbols with all their n-grams computed
material with a hole to its left is assumed to have its n-grams to the left already computed, likewise for the right side"""

# temporary area used by lookup_words
cdef int bufsize
cdef int *buf, *mbuf

bufsize = 5
buf = <int *>malloc(bufsize*sizeof(int))
mbuf = <int *>malloc(
bufsize*sizeof(int))

cdef lookup_words1(nplm.NeuralLM ngram, rule.Phrase ewords, antstates, char mapdigits, bint lowercase, float ceiling):
    cdef int length, edge, i, j
    cdef float sum, p
    cdef int word
    global buf, mbuf, bufsize

    length = ewords.n - ewords.n_vars
    for words in antstates:
        i = len(words)
        length = length + i
    while length > bufsize:
        bufsize = bufsize * 2
        buf = <int *>realloc(buf, bufsize*sizeof(int))
        mbuf = <int *>realloc(mbuf, bufsize*sizeof(int))

    i = 0
    # same as rule.Phrase.subst
    for j from 0 <= j < ewords.n:
        if sym.isvar(ewords.syms[j]):
            for word in antstates[sym.getindex(ewords.syms[j])-1]:
                buf[i] = word
                i = i + 1
        else:
            buf[i] = ewords.syms[j]
            i = i + 1

    for i from 0 <= i < length:
        if not sym.isvar(buf[i]) and buf[i] != HOLE:
            mbuf[i] = map_word(ngram, buf[i], mapdigits, lowercase)

    sum = 0.0

    edge = 0
    remnant = []

    for i from 0 <= i < length:
        if buf[i] == HOLE:
            edge = i+1
        elif i-edge+1 >= ngram.order:
            p = -ngram.c_lookup_ngram(&mbuf[i-ngram.order+1], ngram.order)
            if p > ceiling:
                p = ceiling
            sum = sum + p
        elif edge == 0:
            remnant.append(buf[i])

    if edge != 0 or length >= ngram.order:
        remnant.append(HOLE)
        if edge < length - (ngram.order-1):
            edge = length - (ngram.order-1)
        for i from edge <= i < length:
            remnant.append(buf[i])
    return (tuple(remnant), sum)

# temporary area used by map_word
cdef int wordbufsize
cdef char *wordbuf
wordbufsize = 100
wordbuf = <char *>malloc(wordbufsize)

# global cache. won't work for multiple language models
cdef int *word_cache
cdef int word_cachesize
word_cachesize = 0

cdef int map_word(nplm.NeuralLM ngram, int s, char c, bint lc):
    """Assumes that ids in the LM's vocab are nonnegative!"""
    cdef char *word
    cdef int i, n, result
    cdef int oldcachesize
    global wordbuf, wordbufsize
    global word_cache, word_cachesize

    if s > word_cachesize-1:
        oldcachesize = word_cachesize
        word_cachesize = max(int(word_cachesize * 1.1), s+1)
        if oldcachesize == 0:
            word_cache = <int *>malloc(word_cachesize*sizeof(int))
        else:
            word_cache = <int *>realloc(word_cache, word_cachesize*sizeof(int))
        for i from oldcachesize <= i < word_cachesize:
            word_cache[i] = -1
    
    if <int>word_cache[s] < 0:
        word = <char *>sym.tostring(s)
        n = len(word)
        if n+1 > wordbufsize:
            wordbuf = <char *>realloc(wordbuf, (n+1)*sizeof(char))
            wordbufsize = n+1
            
        # copy word and map digits
        for i from 0 <= i < n:
            if c and c'0' <= word[i] <= c'9':
                wordbuf[i] = c
            else:
                wordbuf[i] = word[i]

            if lc:
                wordbuf[i] = tolower(wordbuf[i])
        wordbuf[n] = c'\0'

        word_cache[s] = ngram.c_lookup_word(wordbuf)
    
    return word_cache[s]

LOGZERO = -999.0 # BIGLM's is -99, but that shouldn't affect us

class LanguageModel(model.Model):
    def __init__(self, filename, feat, order=3, mapdigits=0, lowercase=False, ceiling=-LOGZERO, cache_size=0):
        cdef char *tmpstr
        model.Model.__init__(self)

        self.feat = feat

        self.positive = True
        self.factorable = True

        if type(mapdigits) is str:
            tmpstr = mapdigits
            self.mapdigits = tmpstr[0]
        else:
            self.mapdigits = mapdigits
        self.lowercase = lowercase
        self.ceiling = ceiling

        self.ngram = nplm.NeuralLM(cache_size=cache_size)
        self.ngram.read(filename)

        self.order = self.ngram.order
        self.toprule = rule.Phrase((START,)*(self.order-1) + (sym.setindex(sym.fromtag('S'), 1), STOP))

    ### Decoder interface
        
    def transition(self, r, antstates, i, j, j1=None):
        #log.write("transition(%s, %s)\n" % (r, ", ".join(map(self.strstate, antstates))))
        (state, dcost) = lookup_words1(self.ngram, r.e, antstates, self.mapdigits, self.lowercase, self.ceiling)
        return (state, svector.Vector(self.feat, dcost))

    def bonus(self, lhs, state):
        return svector.Vector()

    def estimate(self, r):
        return svector.Vector()

    def finaltransition(self, state):
        _, dcost = lookup_words1(self.ngram, self.toprule, (state,), self.mapdigits, self.lowercase, self.ceiling)
        return svector.Vector(self.feat, dcost)

    def strstate(self, state):
        result = []
        prev = None
        for e in state:
            if e == HOLE:
                result.append("...")
            else:
                if prev and prev != HOLE:
                    result.append(" ")
                result.append(sym.tostring(e))
            prev = e
        return "".join(result)
