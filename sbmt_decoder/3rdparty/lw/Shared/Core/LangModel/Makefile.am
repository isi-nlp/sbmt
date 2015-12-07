noinst_LTLIBRARIES = liblwcorelangmodel.la      

srcfiles =   CountsMerger.h   impl/CustomizerLM.h               impl/LangModelImplBase.h  impl/Prob.h \
  CountsTrainer.h         impl/CustomizerLMTrainer.cpp      impl/LangModelImpl.cpp    impl/Trace.cpp \
  CustomizerLMTrainer.h   impl/LangModelBDB.cpp             impl/LangModelImpl.h      impl/Trie.cpp \
  impl/BinHashMap.cpp     impl/LangModelBDBDatabase.cpp     impl/LangModelKN.cpp      impl/Trie.h \
  impl/BinHashMap.h       impl/LangModelBDBDatabase.h       impl/LangModelKN.h        LangModelFactory.h \
  impl/CountsMerger.cpp   impl/LangModelBDB.h               impl/LangModelSA.cpp      LangModel.h \
  impl/CountsReader.cpp   impl/LangModel.cpp                impl/LangModelSA.h        LangModelHistoryFactory.h \
  impl/CountsReader.h     impl/LangModelFactory.cpp         impl/LangModelSANode.cpp  LangModelHistory.h \
  impl/CountsTrainer.cpp  impl/LangModelHistory.cpp         impl/LangModelSANode.h    Trace.h \
  impl/CustomizerLM.cpp   impl/LangModelHistoryFactory.cpp  impl/LMVersion.h

CPPFLAGS = -D__GCC3__ -DSLM_SWAP_BYTES -DQT_THREAD_SUPPORT -D_DECODER \
           -DVOCAB_USE_STD_MAP -DLM_NO_BDB_LIB -DLM_NO_COMMON_LIB -DNO_QT -DNOTRACE

liblwcorelangmodel_la_SOURCES = $(srcfiles)

INCLUDES = -I@top_srcdir@/3rdparty/lw/Shared/Core/LangModel \
           -I@top_srcdir@/3rdparty/lw/Shared/Core   \
           -I@top_srcdir@/3rdparty/lw/Shared \
           -I@top_srcdir@/3rdparty/lw  



