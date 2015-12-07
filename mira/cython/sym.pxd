cimport std
cimport numberizer

cdef extern from "sym.hpp":
    ctypedef struct sym:
        int type
        int num
        int index

    # constants
    int terminal "sym::terminal", nonterminal "sym::nonterminal"
    int no_index "sym::no_index"
    
    ctypedef struct c_alphabet "alphabet":
        numberizer.c_numberizer terminals, nonterminals
        std.c_string sym_to_string(sym)
        sym string_to_sym(char *s)
        std.c_string sym_to_cat(sym)
        sym cat_to_sym(char *s)

cpdef std.const_char_ptr tostring(int x)
cpdef int fromstring(char *s)
cpdef std.const_char_ptr totag(int x)
cpdef int fromtag(char *s)
cpdef bint isvar(int x)
cpdef bint isword(int x)
cpdef int getindex(int x)
cpdef int setindex(int x, int i)
cpdef int clearindex(int x)
cpdef bint match(int x, int y)
