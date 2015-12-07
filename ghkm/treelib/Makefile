#include ../..//Env.mak

## Compilation options

#CXXARGS = -g -I$(CONTRinclude -Iinclude -static
CXXARGS = -O -I../include -Iinclude -static
AR  = ar

## Directories and files

SRCDIR=src
LIBDIR=../lib

LIB=$(LIBDIR)/libtreelib.a

## Build directives

all: $(LIB)

$(LIB): $(SRCDIR)/treelib/TreeNode.o   \
	$(SRCDIR)/treelib/Tree.o   
	ar r $@ $^
	ranlib $@

$(SRCDIR)/treelib/TreeNode.o: $(SRCDIR)/treelib/TreeNode.cpp
	$(CXX) $(CXXARGS) -c -o $@ $<

$(SRCDIR)/treelib/Tree.o: $(SRCDIR)/treelib/Tree.cpp
	$(CXX) $(CXXARGS) -c -o $@ $<

.PHONY: clean

clean:
	/bin/rm -f $(SRCDIR)/treelib/*.o

realclean: clean
	/bin/rm -f $(LIB)
