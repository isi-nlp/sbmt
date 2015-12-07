bin_PROGRAMS = LangModel

LangModel_SOURCES = LangModelMain.cpp
LangModel_LDADD  = @top_builddir@/3rdparty/lw/Shared/Core/LangModel/liblwcorelangmodel.la \
                   @top_builddir@/3rdparty/lw/Shared/Common/liblwcommon.la 
                   

INCLUDES = -I@top_srcdir@/3rdparty/lw/Shared 	\
	       -I@top_srcdir@/3rdparty/lw        \
          -I@top_srcdir@/3rdparty/lw/Shared/Core 

CPPFLAGS = -D__GCC3__ -DSLM_SWAP_BYTES -DQT_THREAD_SUPPORT \
           -DLM_NO_BDB_LIB -DLM_NO_COMMON_LIB -DNO_QT -DNOTRACE
