// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _LANG_MODEL_HISTORY_FACTORY_H
#define _LANG_MODEL_HISTORY_FACTORY_H

#include "Common/MemAlloc.h"

namespace LW {

	class LangModelHistory;

	class LangModelHistoryFactory 
	{
	public:
		LangModelHistoryFactory(unsigned int nMaxNGramSize);
		virtual ~LangModelHistoryFactory();
	public:
		/** 
			Allocates a new instance of LangModelHistory. DO NOT DELETE THIS POINTER.
		*/
		LangModelHistory* create();
	public:
		/** 
			Allocates memory (usually to create new instances of History or for the word history buffer)
		*/
		void* alloc(size_t nSize);
		/**
			Releases a pointer previously allocated by calling allocate
		*/
//		void free(void* pPtr);
	private:
		unsigned int m_nMaxNGramSize;
		MemAlloc m_memAllocator;
	};

}

#endif // _LANG_MODEL_HISTORY_FACTORY_H
