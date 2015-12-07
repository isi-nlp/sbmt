RM=/bin/rm
CXX=g++
XECONTRIBDIR=../..
##############################################################
# Relative directories
##############################################################

### executable directory:
BINDIR=../bin
### sample data:
SAMPLEDIR=../samples
### libraries required by the rule extractor:
LOCALLIBDIR=../libs

##############################################################
# Compilation parameters
##############################################################

### optimization flags:
OPTLEVEL =  -DDEBUG
OPTFLAGS = -fomit-frame-pointer -finline-functions -ffast-math -funroll-loops
# debug mode:
#OPTFLAGS += -g3 -DDEBUG
#OPTFLAGS += -O3 -DDEBUG -static
OPTFLAGS += -O3 -DDEBUG

### compilation flags:
CXXFLAGS = $(OPTFLAGS) -ftemplate-depth-30 -Wall \
            -DGZSTREAM_NAMESPACE=gz
LINKFLAGS = \
            -L$(XECONTRIBDIR)/treelib/lib \
            -L$(XECONTRIBDIR)/gzstream \
	    -L$(XECONTRIBDIR)/static \
            -L$(XECONTRIBDIR)/lib \
            -Wl,-R$(XECONTRIBDIR)/lib \
            -ltreelib \
            -lz -ldb -ldb_cxx -lpthread -lgzstream \
	    -lpcre++ \


INCLUDEFLAGS= \
             -I$(XECONTRIBDIR)/treelib/include \
             -I$(XECONTRIBDIR)/gzstream \
             -I$(XECONTRIBDIR)/db-4.3.28/include \
             -I$(XECONTRIBDIR)/nstring \
             -I$(XECONTRIBDIR)/include

##############################################################
# Targets
##############################################################

OBJS = \
	Tree.o \
	TreeNode.o \
	State.o \
	RuleInst.o \
	RuleNode.o \
	RuleDescriptor.o \
	RuleRHS.o \
	RuleRHS_el.o \
	RuleSet.o \
	Derivation.o \
	DerivationNode.o \
	DerivationNodeDescriptor.o \
	LabelSpan.o \
	Alignment.o \
	Variable.o \
	ATS.o \
	GenDer.o \
	WeightF.o \
	MyErr.o \
	db_access.o \
	TreeProjector.o

SHAREDOBJS = 

HEADERS = \
	hashutils.h \
	defs.h \
	Features.h

EXTRACT=$(BINDIR)/extract

APPS=$(BINDIR)/extract \
     $(BINDIR)/index_db \
     $(BINDIR)/create_db \
     $(BINDIR)/query_db \
     $(BINDIR)/dump_db \
     $(BINDIR)/add_weights \
     $(BINDIR)/merge_weights \
     $(BINDIR)/create_em_weights \
     $(BINDIR)/print_rules \
     $(BINDIR)/print_rules_fast \
     $(BINDIR)/print_norm_groups \
     $(BINDIR)/add_norm_counts \
     $(BINDIR)/add_probs


##############################################################
# Build directives
##############################################################

all: 
	make $(APPS)

.SECONDARY:

$(BINDIR)/%: $(OBJS) %.o
	$(CXX) $(OPTLEVEL) $(CXXFLAGS) $(*F).o $(SHAREDOBJS) $(OBJS) -o $@ $(LINKFLAGS)

# Note: for some reason, TreeNode.o crashes if compiled with -Ox, 
# where x >= 2. Here, flag -O overrides -O3. 
TreeNode.o: TreeNode.C $(HEADERS)
	$(CXX) -O $(CXXFLAGS) $(INCLUDEFLAGS) -c $<
%.o: %.C $(HEADERS)
	$(CXX) $(OPTLEVEL) $(CXXFLAGS) $(INCLUDEFLAGS) -c $<

%.ps: %.h
	enscript -Ec -o - $< | mpage -2 -t -R > $@
%.C.ps: %.C
	enscript -Ec -o - $< | mpage -2 -t -R > $@

##############################################################
# Directives to compile, debug, and test
##############################################################

# Parameters to just extract derivations from the 1st 10 lines:
EXTRACTARGS = -d - -s 1 -e 10

.PHONY: debug minimaldemo composeddemo ATSdemo rulereader check

# Debugging with valgrind: make sure that optimization options
# are set to '-g3 -DDEBUG':
debug: $(EXTRACT)
	valgrind --show-reachable=yes --leak-check=yes --skin=memcheck \
	$(LIBENV) $< -l 0:0 $(EXTRACTARGS)

# Just a test case to show how to extract (locally indexed) necessary rules
# with corresponding derivations.
minimaldemo: $(EXTRACT)
	$(LIBENV) $< -l 1000:0 $(EXTRACTARGS) 2> /dev/null

# Same here, but with composed rules (LHS of size <= 4).
composeddemo: $(EXTRACT)
	$(LIBENV) $< -l 1000:4 $(EXTRACTARGS) 2> /dev/null

# Same for ATS rule extraction:
ATSdemo: $(EXTRACT)
	$(LIBENV) $< -r $(SAMPLEDIR)/comingfrom4 \
	-a $(SAMPLEDIR)/my-ATS2 2> /dev/null

# Try reading rules: (still not functional)
rulereader: $(EXTRACT)
	$(LIBENV) $< -l 0:0 $(EXTRACTARGS) 2> /dev/null | \
	$(BINDIR)/print_rule_info 

# Check that the installation worked properly:
check: $(EXTRACT)
	make -s minimaldemo  | diff -q -s $(SAMPLEDIR)/OUT1 -
	make -s composeddemo | diff -q -s $(SAMPLEDIR)/OUT2 -

##############################################################
# Cleanup stuff
##############################################################

.PHONY: clean realclean

clean:
	$(RM) -f *.o *~ core core.* gmon.out

realclean: clean
	cd $(XECONTRIBDIR)/treelib && make clean
	$(RM) -f $(APPS) 
