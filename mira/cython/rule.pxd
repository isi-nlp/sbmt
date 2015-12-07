cdef class Phrase:
    cdef int *syms
    cdef int n, *varpos, n_vars
    cdef int chunkpos(self, int k)
    cdef int chunklen(self, int k)

cdef class Rule:
    cdef public int lhs
    cdef readonly Phrase f, e
    cdef public scores
    cdef public attrs
    cdef public statelesscost
    
    
