#
# Makefile 
#
INSTALL	= install
RM	= rm -f
CC	= gcc
CFLAGS	= -O
LD	= $(CC)
LOADLIBES = -lm
SYSTEM  = $(shell uname -s | tr '[a-z]' '[A-Z]' | tr -d '_ -/')
VERSION = $(shell uname -r)
MARCH	= $(shell uname -m | tr '[A-Z]' '[a-z]')
MAJOR   = $(firstword $(subst ., ,$(VERSION)))
MINOR   = $(strip $(word 2,$(subst ., ,$(VERSION))))
LFS_CFLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE 
LFS_LDFLAGS = 

ifeq (SUNOS,${SYSTEM})
ifeq (5,${MAJOR})
# use these for the SUN CC compiler
CC	= cc
LD	= $(CC)
## SPARCv7
LFS_CFLAGS = $(shell getconf LFS_CFLAGS 2>>/dev/null)
LFS_LDFLAGS = $(shell getconf LFS_LDFLAGS 2>>/dev/null)
V7FLAGS	= -xtarget=generic
V9FLAGS	= -xtarget=ultra -xarch=v9
CFLAGS  = -dalign -ftrap=%none -fsimple -xlibmil $(EXTRACFLAGS)
#EXTRACFLAGS = $(V7FLAGS)
CFLAGS := -DSOLARIS $(CFLAGS) -xO4 -D__EXTENSIONS__=1
LOADLIBES += -lnsl -lsocket
INSTALL = /usr/ucb/install
else
# old Solaris 1 not supported!
endif
endif

ifeq (IRIX64,${SYSTEM})
# The regular 64bit Irix stuff is just too slow, use n32!
SYSTEM        := IRIX
endif

ifeq (IRIX,${SYSTEM})
CC      = cc -n32 -mips3 -r4000
LD      = $(CC)
OPT_NORM = -O3 -IPA -LNO:opt=1
endif

ifeq (AIX,${SYSTEM})
CC	= xlc
CXX	= xlC
endif

ifeq (DARWIN,${SYSTEM})
#CFLAGS	+= -DDEBUG_WAIT=1 -DDEBUG_EVENTLOOP=1 
CFLAGS	+= -DMUST_USE_SELECT_NOT_POLL=1
endif

ifeq (LINUX,${SYSTEM})
ifeq (ia64,${MARCH})
CFLAGS	= -m32 -march=pentium4 -Wall -O2 -ggdb
else
ifeq (x86_64,${MARCH})
CFLAGS	= -m32 -march=pentium4 -Wall -O2 -ggdb
else
CFLAGS	= -Wall -O2 -march=pentium4 -ggdb
endif
endif
EXTRA_LDFLAGS += -m32 -ggdb
##CFLAGS	+= -D__USE_POSIX=199309
LOADLIBES := -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic $(LOADLIBES)
# LOADLIBES += -lstdc++
ifeq (2,${MAJOR})
ifeq (4,${MINOR})
CFLAGS += -DUSE_MEMINFO=1
endif
ifeq (6,${MINOR})
CFLAGS += -DUSE_MEMINFO=1
endif
endif
# CFLAGS += -DEXTRA_DEBUG=1
LFS_CFLAGS = $(shell getconf LFS_CFLAGS 2>>/dev/null)
LFS_LDFLAGS = $(shell getconf LFS_LDFLAGS 2>>/dev/null)
endif

#
# === [3] ======================================================= rules section
# There is no need to change things below this line.
CFLAGS += -D${SYSTEM} -DMAJOR=${MAJOR} -DMINOR=${MINOR}

# add large file support
ifneq (,${LFS_CFLAGS})
CFLAGS += $(LFS_CFLAGS)
endif
ifneq (,${LFS_LDFLAGS})
LDFLAGS += $(LFS_LDFLAGS)
endif
ifneq (,${EXTRA_LDFLAGS})
LDFLAGS += $(EXTRA_LDFLAGS)
endif

CXXFLAGS = $(CFLAGS)

all : scoreTranslation

scoreTranslation.o: scoreTranslation.cc Array2.hh mystl.hh my_namespace.hh \
	bleu.hh
scoreTransation: scoreTranslation.o
	$(LD) $(LDFLAGS) $(CFLAGS) $^ -o $@ $(LOADLIBES)

clean:
	$(RM) *.o $(GENDOC) core core.*

distclean: clean
	$(RM) scoreTranslation

