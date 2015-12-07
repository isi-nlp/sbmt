// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved

#ifndef _MEM_ALLOC_H_
#define _MEM_ALLOC_H_

#include <vector>
#include <cstddef>

namespace LW {

/**
This class is an efficient (as in size) memory allocator.
It avoids the overhead of a regular heap.
This allocator is supposed to be used when:
a. A large number of relatively small blocks are required
b. The blocks only need to be disposed of at the same time.
*/
class MemAlloc {
public:
	static const std::size_t DEFAULT_ALLOCATION_INCREMENT = 1024 * 1024;
public:
	/// See the comment for m_nAllocIncrement
	MemAlloc(std::size_t nAllocIncrement = DEFAULT_ALLOCATION_INCREMENT);
	///
	virtual ~MemAlloc();
public:
	/// Allocates a chunk of size nSize
	/// The chunk is not aligned
	void* alloc(std::size_t nSize);
	/// Frees all memory allocated by this allocator
	void freeAll();
private:
	typedef std::vector<char*> AllocVector;
	// Vector of pointers to each chunk allocated
	AllocVector m_blocks;
	/// Pointer to the current block we allocate memory from
	char* m_pCurrentBlock;
	/// Pointer to the end of the current block
	char* m_pCurrentBlockEnd;
	/// This is the size of the blocks we allocate from the heap with malloc()
	/// each time we don't have enough memory to satisfy the current request
	std::size_t m_nAllocIncrement;
};

} // namespace LW

#endif // _MEM_ALLOC_H_
