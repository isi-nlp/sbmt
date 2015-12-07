// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#include <cstdlib>
#include <stdio.h>
#include <vector>

#include "Common/MemAlloc.h"

using namespace std;

namespace LW {

MemAlloc::MemAlloc(size_t nAllocIncrement)
{
	m_pCurrentBlock = NULL;
	m_pCurrentBlockEnd = NULL;
	m_nAllocIncrement = nAllocIncrement;
}

MemAlloc::~MemAlloc()
{
	freeAll();
}
void* MemAlloc::alloc(size_t nSize) 
{
	// Do we have enough room inside the current block?
	if ((size_t) (m_pCurrentBlockEnd - m_pCurrentBlock) < nSize) {
		// Nope, must allocate another block
		size_t nAllocIncrement = m_nAllocIncrement;
		// Check to see if the requested size is larger
		// than the size of the block we normally allocate
		if (nSize > nAllocIncrement) {
			nAllocIncrement = nSize;
		}
		m_pCurrentBlock = (char*) malloc(nAllocIncrement);
		// Add the block to the list so we can delete it
		m_blocks.push_back(m_pCurrentBlock);

		m_pCurrentBlockEnd = m_pCurrentBlock + nAllocIncrement;
	}
	// We have enough room now
	char* pReturn = m_pCurrentBlock;
	m_pCurrentBlock += nSize;

	return pReturn;
}

void MemAlloc::freeAll() 
{
	AllocVector::iterator it;
	for (it = m_blocks.begin(); it != m_blocks.end(); it++) {
		free(*it);
	}
	m_pCurrentBlock = NULL;
	m_pCurrentBlockEnd = NULL;
}

} // namespace LW
