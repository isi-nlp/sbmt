// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include "LangModel/LangModelHistoryFactory.h"

#include "Common/MemAlloc.h"
#include "LangModel/LangModelHistory.h"

using namespace std;

namespace LW {
	LangModelHistoryFactory::LangModelHistoryFactory(unsigned int nMaxNGramSize)
	{
		m_nMaxNGramSize = nMaxNGramSize;
	}

	LangModelHistoryFactory::~LangModelHistoryFactory()
	{
	}

	LangModelHistory* LangModelHistoryFactory::create()
	{
		// Allocate memory for object
		LangModelHistory* pLangModelHistory = (LangModelHistory*) m_memAllocator.alloc(sizeof(LangModelHistory));

		// Allocate word history buffer
		LWVocab::WordID* pWordHistory = (LWVocab::WordID*) m_memAllocator.alloc(m_nMaxNGramSize * sizeof(LWVocab::WordID));

		// Call the constructor "in place"
		new (pLangModelHistory) LangModelHistory(m_nMaxNGramSize, pWordHistory);

		return pLangModelHistory;
	}

	void* LangModelHistoryFactory::alloc(size_t nSize)
	{
		return m_memAllocator.alloc(nSize);
	}

	//void LangModelHistoryFactory::free(void* pPtr)
	//{
	//	// This allocator cannot deallocate one single block only
	//	// Do nothing, the pointer is automatically destroyed when the allocator gets destroyed
	//}
} // namespace LW
