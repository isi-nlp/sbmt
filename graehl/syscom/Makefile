DIR=.
SHARED=$(DIR)/../shared
PROGS=syscom

syscom_LIB=$(BOOST_OPTIONS_LIB)
syscom_NOTEST=1
syscom_NOSTATIC=1
syscom_SRC=syscom.cpp in_memory_token_storage.cpp
#logmath_io.cpp

SBMT=$(TRUNK)/sbmt_decoder
SBMTSRC=$(SBMT)/src

vpath %.cpp . $(SHARED) $(SBMTSRC)
VPATH = $(SHARED):$(SBMTSRC)

INC=. $(TRUNK) $(SBMT)/include $(TRUNK)/gusc
LIB=
CXX:=g++

CXXFLAGS_COMMON= -ggdb -ffast-math -Wno-parentheses -fopenmp
CPPFLAGS += -DNO_BACKTRACE -DUSE_NONDET_RANDOM
CXXFLAGS += -O

default: all

include ../shared/graehl.mk
