# distutils: language = c++

cimport std
cimport numberizer
import sys

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

cdef c_alphabet cab
cdef std.c_string buf

cpdef std.const_char_ptr tostring(int x):
    cdef sym sx = <sym>x
    cab.sym_to_string(<sym>x).swap(buf)
    return buf.c_str()

cpdef int fromstring(char *s):
    return <int>cab.string_to_sym(s)

cpdef std.const_char_ptr totag(int x):
    cdef sym sx = <sym>x
    cab.sym_to_cat(<sym>x).swap(buf)
    return buf.c_str()

cpdef int fromtag(char *s):
    return <int>cab.cat_to_sym(s)

cpdef bint isvar(int x):
    cdef sym sx = <sym>x
    return sx.type == nonterminal

cpdef bint isword(int x):
    cdef sym sx = <sym>x
    return sx.type == terminal

cpdef int getindex(int x):
    cdef sym sx = <sym>x
    return sx.index

cpdef int setindex(int x, int i):
    cdef sym y = <sym>x
    y.index = i
    return <int>y

cpdef int clearindex(int x):
    cdef sym y = <sym>x
    y.index = no_index
    return <int>y

cpdef bint match(int x, int y):
    cdef sym sx = <sym>x
    cdef sym sy = <sym>y
    return sx.num == sy.num

def nonterminals():
    cdef int i
    cdef sym x
    x.type = nonterminal
    l = []
    for cab.nonterminals.begin_index() <= i < cab.nonterminals.end_index():
        x.num = i
        l.append(<int>x)
    return l
