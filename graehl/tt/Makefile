#gdb --args /cache/tt/bin/cygwin/forest-em.debug -f /cache/tt/sample/best_forest -I /cache/tt/sample/best_weights -n /cache/tt/sample/best_norm -i 1 -m 100k -w 3 -x sample/best_viterbi -L 9

PROGS = treeviz tt treetrie sortpercent

treetrie_SRC=treetrie.cpp
treetrie_LIB=$(BOOST_OPTIONS_LIB)
treetrie_NOSTATIC=1
treetrie_NOTEST=1

tt_SRC=tt.cpp
tt_NOTEST=1

#NOSTATIC=1

sortpercent_SRC=sortpercent.cpp
sortpercent_LIB=$(BOOST_OPTIONS_LIB)
sortpercent_NOSTATIC=1
sortpercent_NOTEST=1

text-to-cc_SRC=text-to-cc.cpp
text-to-cc_NOTEST=1
text-to-cc_NOSTATIC=1
text-to-cc_NODEBUG=1

treeviz_SRC=treeviz.cpp
treeviz_LIB=$(BOOST_OPTIONS_LIB)
treeviz_NOTEST=1
#treeviz_NOSTATIC=1
treeviz_NODEBUG=1


INC= . $(TRUNK)
LIB=
#CXX:=g++

CXXFLAGS_COMMON= -ggdb -ffast-math
CPPFLAGS += -DNO_BACKTRACE -DUSE_NONDET_RANDOM
CXXFLAGS += -O
#-DSINGLE_PRECISION
## would have to link with boost random nondet source

CPPFLAGS_DEBUG+= -DDEBUG
# -DDEBUGFIXEDINPUT
CPPFLAGS_TEST+= -DTEST -DDEBUG

#CPP_EXT=cpp
ALL_CLEAN += treetrie.README.hpp *.restart.* *.swap.* *.stackdump *.d *.out *.log massif.* core

default: all

include ../shared/graehl.mk
vpath %.cpp .:$(SHARED)


mydefault: $(BIN)/sortpercent.debug
#debug
		$(BIN)/sortpercent.debug -p .3 -m -H  -i sample/100.counts
		$(BIN)/sortpercent.debug -R -p .3 -m -H  -i sample/100.counts

sortpercent-test: $(BIN)/sortpercent
	$< -n -i sample/best_weights -R


treeviz-test: $(BIN)/treeviz
	$< sample/trees > sample/trees.dot
	dot -Tps sample/trees.dot -o sample/trees.ps

sample-em: sample-em.cpp
	g++ -o $(BIN)/sample-em sample-em.cpp
	$(BIN)/sample-em 1 9 1 9

treetrie.README.hpp: treetrie.README $(BIN)/text-to-cc
	$(BIN)/text-to-cc usage_str < $< > $@

depend: treetrie.README.hpp
