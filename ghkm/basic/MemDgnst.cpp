// Copyright (c) 2001, 2002, 2003, 2004, 2005, Language Weaver Inc, All Rights Reserved
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "MemDgnst.h"

char *MemDgnst::memp;
MemDgnst::mem_header *MemDgnst::memp_start;
char *MemDgnst::memp_end;
size_t MemDgnst::total_size = 0;

void
MemDgnst::more_core ()
{
  const size_t size = 30*1024* 1024;
  void *core = malloc (size);
  if (!core) {
    fprintf (stderr, "out of memory\n");
    abort ();
  }
  total_size += size;

  mem_header *nmhp = static_cast<mem_header *> (core);
  nmhp->next = memp_start;
  nmhp->size = size;
  memp_start = nmhp;
  memp = reinterpret_cast<char *> (&memp_start[1]);
  memp_end = reinterpret_cast<char *> (memp_start) + memp_start->size;
}

void
MemDgnst::clear_memory ()
{
  mem_header *mhp = memp_start;
  while (mhp) {
    mem_header *nmhp = mhp->next;
    free (mhp);
    mhp = nmhp;
  }
  memp = NULL;
  memp_start = NULL;
  memp_end = NULL;
  total_size = 0;
}
