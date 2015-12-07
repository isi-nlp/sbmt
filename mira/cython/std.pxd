cdef extern from *:
    ctypedef char* const_char_ptr "const char*" # Cython FAQ

cdef extern from "std.hpp":
    pass

cdef extern from "string":
    ctypedef struct c_string "std::string":
        void assign(char *)
        void swap(c_string)
        char *c_str ()
