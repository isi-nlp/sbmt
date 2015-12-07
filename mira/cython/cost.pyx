# distutils: language = c++

cdef extern from "logp.h":
    double neglog10p_add(double x, double y)
    double neglog10p_subtract(double x, double y)

cdef extern from "errno.h":
    int errno
    int EDOM, ERANGE

try:
    IMPOSSIBLE=float('inf')
except ValueError:
    IMPOSSIBLE=1000000.

import math

def fromprob(p):
    try:
        return -math.log10(p)
    except (ValueError, OverflowError):
        return IMPOSSIBLE

def prob(x):
    return math.pow(10.,-x)

def add(double x, double y):
    global errno
    errno = 0
    cdef double z
    z = neglog10p_add(x, y)
    if errno == EDOM:
        raise ValueError
    elif errno == ERANGE:
        raise OverflowError
    return z

def subtract(double x, double y):
    global errno
    errno = 0
    cdef double z
    z = neglog10p_subtract(x, y)
    if errno == EDOM:
        raise ValueError
    elif errno == ERANGE:
        raise OverflowError
    return z
