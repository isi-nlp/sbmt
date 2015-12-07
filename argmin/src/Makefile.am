bin_PROGRAMS = argmin
argmin_SOURCES = argmin.cpp Agenda Minimizer.cpp Chart.cpp \
	Grammar.cpp gpl/Debug.cpp gpl/stats.cpp Derivation.cpp gpl/NBest.hpp \
	PriorityQueueAgenda.cpp CyclingAgenda.cpp gpl/Optional.hpp SetAgenda.cpp

# set the include path found by configure
INCLUDES= $(all_includes)

AM_CXXFLAGS = -ansi -pedantic -Wall

AM_CXXFLAGS += -O3

# gprof
#AM_CXXFLAGS += -pg

# Google profiler
#AM_LDFLAGS += -lprofiler

# Define NO_LM to override use of ngram LM info.
#AM_CXXFLAGS += -DNO_LM

# the library search path.
argmin_LDFLAGS = $(all_libraries) -lsbmt -lboost_iostreams -lboost_date_time -lboost_program_options -lboost_filesystem
noinst_HEADERS = Derivation.hpp Minimizer.hpp \
			Agenda.hpp Chart.hpp gpl/Tiebreak.hpp types.hpp Grammar.hpp \
			gpl/Debug.hpp gpl/stats.hpp gpl/onullstream.hpp throw.hpp \
			PriorityQueueAgenda.hpp CyclingAgenda.hpp SetAgenda.hpp
