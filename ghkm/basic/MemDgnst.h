// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
/*
 * Class for memory diagnostics.
 */
#ifndef _MemDgnst_H_
#define _MemDgnst_H_
#include "LiBEDebug.h"
#include <cassert>
//! Memory allocation class.
class MemDgnst : public LiBEDebug {
public:
  virtual ~MemDgnst() {}

  inline void *operator new (size_t n) {
    char *ret = memp;
    memp += n;
    if (memp <= memp_end)
      return ret;
    more_core ();
    ret = memp;
    memp += n;
    assert(memp <= memp_end);
    return ret;
  }
  
  inline void* operator new(std::size_t, void* __p) throw()
  { return __p; }
  

  inline void operator delete (void *) {}
  static void clear_memory ();

  static size_t total_mem_size() { return total_size;}

private:
  struct mem_header {
    mem_header *next;
    size_t size;
  };
  static char *memp;
  static mem_header *memp_start;
  static char *memp_end;
  static void more_core ();
  static size_t total_size;
};

#endif/* _MemDgnst_H_*/

