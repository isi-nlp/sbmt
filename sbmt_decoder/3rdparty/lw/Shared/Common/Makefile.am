noinst_LTLIBRARIES = liblwcommon.la

srcfiles = Assert.h         impl/logprob.cpp      Mutex.h                              Serializer/Serializer.h \
Atomic.h                    impl/MemAlloc.cpp     os-dep.h                             SmartPtr.h \
ConditionVariable.h         impl/ParamParser.cpp  ParamParser.h                        stldefs.h \
ErrorCode.h                 impl/Platform.cpp     Platform.h                           TraceCommon.h \
FileOp.h                    impl/Trace.cpp        RefCounted.h                         Trace.h \
handle.h                    impl/Util.cpp         Serializer/HeaderIOSerializer.h  \
impl/Assert.cpp             Log.h                 Serializer/SerializerBasicTypes.cpp  Util.h \
impl/ConditionVariable.cpp  logprob.h             Serializer/SerializerBasicTypes.h    Vocab/VocabFactory.h \
impl/ErrorCode.cpp          impl/Mutex.cpp        MemAlloc.h       \
Serializer/Serializer.cpp   Vocab/Vocab.h         Vocab/impl/Vocab.cpp                 Vocab/impl/VocabFactory.cpp



CPPFLAGS = -D__GCC3__ -DSLM_SWAP_BYTES -DQT_THREAD_SUPPORT \
           -DLM_NO_BDB_LIB -DLM_NO_COMMON_LIB -DNO_QT -DNOTRACE
           
liblwcommon_la_SOURCES = $(srcfiles)

liblwcommon_la_LIBADD = -lpthread
           
INCLUDES = -I@top_srcdir@/3rdparty/lw/Shared/Common   \
           -I@top_srcdir@/3rdparty/lw/Shared \
           -I@top_srcdir@/3rdparty/lw
