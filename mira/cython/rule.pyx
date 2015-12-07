# distutils: language = c++
# -*- mode: python; -*-

# rule.pyx
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

cdef extern from "stdlib.h":
   void *malloc(int size)
   void *calloc(int nelem, int size)
   void *realloc(void *buf, int size)
   void free(void *ptr)
   float strtof(char *nptr, char **endptr)
   int strtol(char *nptr, char **endptr, int base)

cdef extern from "strutil.h":
   char *strstrsep(char **stringp, char *delim)
   char *strip(char *s)
   char **split(char *s, char *delim, int *pn)

cdef extern from "string.h":
   char *strsep(char **stringp, char *delim)
   char *strcpy(char *dest, char *src)
   char *strrchr(char *s, int c)

   int strlen(char *s)

import sys
import re
attrs_re = re.compile("""\s*([^\s=]+)=({{{(.*?)}}}|(\S+))(\s|$)""")

import svector
import sym
cimport sym

global span_limit
span_limit = None

cdef int bufsize
cdef char *buf
bufsize = 100
buf = <char *>malloc(bufsize)
cdef ensurebufsize(int size):
   global buf, bufsize
   if size > bufsize:
      buf = <char *>realloc(buf, size*sizeof(char))
      bufsize = size
      
"""def cook_fwords(fwords, maxabslen=None):
     substrings = {}
     for i from 0 <= i < len(fwords)+1:
         for j from i <= j < min(i+maxabslen,len(fwords)+1):
             substring = tuple(fwords[i:j])
             substrings.setdefault(substring, [])
             substrings[substring].append(i)
     return substrings"""

cdef class CookedWords:
   cdef int n, *suffixes, *words

   def __cinit__(self, pywords):
      self.words = <int *>malloc(len(pywords)*sizeof(int))
      self.suffixes = <int *>malloc((len(pywords)+1)*sizeof(int))
   
   def __init__(self, pywords):
      # build a suffix array
      # this is SO inefficient but it doesn't matter much
      cdef int i

      self.n = len(pywords)

      for i from 0 <= i < len(pywords):
         self.words[i] = pywords[i]

      suffixes = []
      for i from 0 <= i <= len(pywords):
         suffixes.append((pywords[i:], i))
      suffixes.sort()
      for i from 0 <= i <= len(pywords):
         self.suffixes[i] = suffixes[i][1]

   def __dealloc__(self):
      free(self.suffixes)
      free(self.words)

   cdef int compare(self, int i, int *key, int key_len):
      cdef int j, k
      j = self.suffixes[i]
      for k from 0 <= k < key_len:
         if j+k >= self.n:
            return -1
         if self.words[j+k] < key[k]:
            return -1
         elif self.words[j+k] > key[k]:
            return 1
      return 0
   
   cdef int find_start(self, int *key, int key_len):
      cdef int high, low, i, c
      low = -1
      high = self.n+1
      while high-low > 1:
         i = (high+low)/2
         c = self.compare(i, key, key_len)
         if c < 0:
            low = i
         else:
            high = i
      if high == self.n+1 or self.compare(high, key, key_len) != 0:
         return -1
      else:
         return high

   cdef int find_stop(self, int *key, int key_len):
      cdef int high, low, i, c
      low = -1
      high = self.n+1
      while high-low > 1:
         i = (high+low)/2
         c = self.compare(i, key, key_len)
         if c > 0:
            high = i
         else:
            low = i
      if low == -1 or self.compare(low, key, key_len) != 0:
         return -1
      else:
         return low+1

   cdef find_all(self, int *key, int key_len):
      cdef int high, low, savelow, i, start, stop, c
      low = -1
      high = self.n+1
      while high-low > 1:
         i = (high+low)/2
         c = self.compare(i, key, key_len)
         if c > 0:
            high = i
         elif c < 0:
            low = i
         else:
            break

      savelow = low
      low = i
      while high-low > 1:
         i = (high+low)/2
         c = self.compare(i, key, key_len)
         if c > 0:
            high = i
         else:
            low = i
      if low == -1 or self.compare(low, key, key_len) != 0:
         return []
      else:
         stop = low

      low = savelow
      high = stop
      while high-low > 1:
         i = (high+low)/2
         c = self.compare(i, key, key_len)
         if c < 0:
            low = i
         else:
            high = i
      if high == self.n+1 or self.compare(high, key, key_len) != 0:
         return []
      else:
         start = high

      result = [None]*(stop-start+1)
      for i from start <= i <= stop: # go in reverse because this makes the indices come out left-to-right
         result[stop-i] = self.suffixes[i]
      return result
            
cdef class Phrase:
   def __cinit__(self, words):
      cdef int i, j, n, n_vars
      cdef char **toks

      n_vars = 0
      if type(words) is str:
         ensurebufsize(len(words)+1)
         strcpy(buf, words)
         toks = split(buf, NULL, &n)
         self.syms = <int *>malloc(n*sizeof(int))
         for i from 0 <= i < n:
            self.syms[i] = sym.fromstring(toks[i])
            if sym.isvar(self.syms[i]):
               n_vars = n_vars + 1
         
      else:
         n = len(words)
         self.syms = <int *>malloc(n*sizeof(int))
         for i from 0 <= i < n:
            if type(words[i]) is str:
               self.syms[i] = sym.fromstring(words[i])
            else:
               self.syms[i] = words[i]
            if sym.isvar(self.syms[i]):
               n_vars = n_vars + 1
      self.n = n
      self.n_vars = n_vars
      self.varpos = <int *>malloc(n_vars*sizeof(int))
      j = 0
      for i from 0 <= i < n:
         if sym.isvar(self.syms[i]):
            self.varpos[j] = i
            j = j + 1

   def __reduce__(self):
      cdef int i
      words = []
      for i from 0 <= i < self.n:
         #words.append(self.syms[i])
         words.append(sym.tostring(self.syms[i]))
      return (Phrase, (words,))

   def __dealloc__(self):
      free(self.syms)
      free(self.varpos)

   def __str__(self):
      strs = []
      cdef int i, s
      for i from 0 <= i < self.n:
         s = self.syms[i]
         strs.append(sym.tostring(s))
      return " ".join(strs)

   def instantiable(self, i, j, n):
      return span_limit is None or (j-i) <= span_limit

   def handle(self):
      """return a hashable representation that normalizes the ordering
      of the nonterminal indices"""
      norm = []
      cdef int i, j, s
      i = 1
      j = 0
      for j from 0 <= j < self.n:
         s = self.syms[j]
         if sym.isvar(s):
            s = sym.setindex(s,i)
            i = i + 1
         norm.append(s)
      return tuple(norm)

   def strhandle(self):
      strs = []
      norm = []
      cdef int i, j, s
      i = 1
      j = 0
      for j from 0 <= j < self.n:
         s = self.syms[j]
         if sym.isvar(s):
            s = sym.setindex(s,i)
            i = i + 1
         norm.append(sym.tostring(s))
      return " ".join(norm)

   def arity(self):
      return self.n_vars

   def getvar(self, i):
      if 0 <= i < self.n_vars:
         return self.syms[self.varpos[i]]
      else:
         raise IndexError()

   cdef int chunkpos(self, int k):
      if k == 0:
         return 0
      else:
         return self.varpos[k-1]+1

   cdef int chunklen(self, int k):
      if self.n_vars == 0:
         return self.n
      elif k == 0:
         return self.varpos[0]
      elif k == self.n_vars:
         return self.n-self.varpos[k-1]-1
      else:
         return self.varpos[k]-self.varpos[k-1]-1

   def getchunk(self, ci):
      cdef int start, stop
      start = self.chunkpos(ci)
      stop = start+self.chunklen(ci)
      chunk = []
      for i from start <= i < stop:
         chunk.append(self.syms[i])
      return chunk

   def stringpos(self, ri, i, j, j1):
      """If phrase spans substring (i,j) with the first variable (of two) ending at j1,
         compute what string position the ri'th symbol would be at."""
      if sym.isvar(self.syms[ri]):
         return None
      if self.n_vars == 0 or ri < self.varpos[0]:
         return i+ri
      elif self.n_vars == 1 or ri > self.varpos[1]:
         return j-self.n+ri
      else:
         return j1+ri-self.varpos[0]-1

   def __cmp__(self, other):
      cdef Phrase otherp
      cdef int i
      otherp = other
      for i from 0 <= i < min(self.n, otherp.n):
         if self.syms[i] < otherp.syms[i]:
            return -1
         elif self.syms[i] > otherp.syms[i]:
            return 1
      if self.n < otherp.n:
         return -1
      elif self.n > otherp.n:
         return 1
      else:
         return 0

   def __hash__(self):
      cdef int i
      cdef unsigned h
      h = 0
      for i from 0 <= i < self.n:
         if self.syms[i] > 0:
            h = (h << 1) + self.syms[i]
         else:
            h = (h << 1) + -self.syms[i]
      return h

   def __len__(self):
      return self.n

   def __getitem__(self, i):
      return self.syms[i]

   def __iter__(self):
      cdef int i
      l = []
      for i from 0 <= i < self.n:
         l.append(self.syms[i])
      return iter(l)

   def subst(self, start, children):
      cdef int i
      for i from 0 <= i < self.n:
         if sym.isvar(self.syms[i]):
            start = start + children[sym.getindex(self.syms[i])-1]
         else:
            start = start + (self.syms[i],)
      return start

   def match(self, CookedWords cooked, maxabslen=None, minhole=0):
      cdef int i, j, i1, j1, i2, j2
      cdef int start[3], stop[3], k0, k1, k2
      matches = []

      for i from 0 <= i < self.n_vars+1:
         start[i] = cooked.find_start(self.syms+self.chunkpos(i), self.chunklen(i))
         if start[i] < 0:
            return []
         stop[i] = cooked.find_stop(self.syms+self.chunkpos(i), self.chunklen(i))

      if self.n_vars == 0:
         for k0 from stop[0] > k0 >= start[0]: # this makes the indexes come out left-to-right
            i = cooked.suffixes[k0]
            matches.append([(i,i+self.chunklen(0))])
      elif self.n_vars == 1:
         for k0 from stop[0] > k0 >= start[0]:
            i = cooked.suffixes[k0]
            i1 = i+self.chunklen(0)
            for k1 from stop[1] > k1 >= start[1]:
               j1 = cooked.suffixes[k1]
               if j1-i1 >= minhole:
                  j = j1+self.chunklen(1)
                  if maxabslen is None or j-i <= maxabslen:
                     matches.append([(i,j), (i1,j1)])
      elif self.n_vars == 2:
         for k0 from stop[0] > k0 >= start[0]:
            i = cooked.suffixes[k0]
            i1 = i+self.chunklen(0)
            for k2 from stop[2] > k2 >= start[2]:
               j2 = cooked.suffixes[k2]
               j = j2+self.chunklen(2)
               if maxabslen is None or j-i <= maxabslen:
                  for k1 from stop[1] > k1 >= start[1]:
                     j1 = cooked.suffixes[k1]
                     i2 = j1+self.chunklen(1)
                     if j1-i1 >= minhole and j2-i2 >= minhole:
                        matches.append([(i,j), (i1,j1), (i2,j2)])

      """chunkmatches = [None]*(self.n_vars+1)
      for k from 0 <= k <= self.n_vars:
         chunkmatches[k] = cooked.find_all(self.syms+self.chunkpos(k), self.chunklen(k))
         if len(chunkmatches[k]) == 0:
            return []

      k = 0
      ii = [0]*(self.n_vars+1)
      while True:
         while k < self.n_vars:
            k = k + 1
            ii[k] = 0

         hi = chunkmatches[0][ii[0]]
         hj = chunkmatches[self.n_vars][ii[self.n_vars]] + self.chunklen(self.n_vars)
         if maxabslen is None or hj-hi <= maxabslen:
            # build the match descriptor
            match = [(hi,hj)]
            for j from 0 <= j < self.n_vars:
               hi = chunkmatches[j][ii[j]]+self.chunklen(j)
               hj = chunkmatches[j+1][ii[j+1]]
               if hj-hi >= minhole:
                  match.append((hi,hj))
               else:
                  break
            else:
               matches.append(match)

         # advance indices
         while k >= 0:
            ii[k] = ii[k] + 1
            if ii[k] >= len(chunkmatches[k]):
               # carry
               k = k - 1
            else:
               break
         if k < 0:
            break"""

      return matches

cdef class Rule:
   def __cinit__(self, lhs, f, e, scores=None, attrs=None):
      cdef int i, n
      cdef char *rest

   def __init__(self, lhs, f, e, scores=None, attrs=None):
      if type(lhs) is str:
         self.lhs = sym.fromstring(lhs)
      else:
         if not sym.isvar(lhs):
            sys.stderr.write("error: lhs=%d\n" % lhs)
         self.lhs = lhs
      self.f = f
      self.e = e
      self.scores = scores or svector.Vector()
      self.attrs = attrs or {}
      self.statelesscost = 0.0

   def __reduce__(self):
      return (Rule, (sym.tostring(self.lhs), self.f, self.e, self.scores, self.attrs))

   def __str__(self):
      fields = [sym.tostring(self.lhs), str(self.f), str(self.e), str(self.scores)]
      if self.attrs is not None:
         avs = []
         for a,v in self.attrs.iteritems():
            if a == 'align': # special treatment for backwards compatibility
               try:
                  v = " ".join(["%s-%s" % (fi,ei) for (fi,ei) in v])
               except:
                  v = str(v)
            else:
               v = str(v)

            if " " in v:
               avs.append("%s={{{%s}}}" % (a,v))
            else:
               avs.append("%s=%s" % (a,v))
         fields.append(" ".join(avs))
      
      return " ||| ".join(fields)

   def __hash__(self):
      return hash((self.lhs, self.f, self.e))

   def __cmp__(self, Rule other):
      return cmp((self.lhs, self.f, self.e, self.attrs), (other.lhs, other.f, other.e, other.attrs))

   def __iadd__(self, Rule other):
      self.scores = self.scores.__iadd__(other.scores)
      return self

   def fmerge(self, Phrase f):
      if self.f == f:
         self.f = f
      
   def arity(self):
      return self.f.arity()

   def to_line(self):
      return self.__str__()

def rule_from_line(line):
   cdef char *cline
   cdef char **fields, *cp
   cdef int n, i
   cdef Rule r
   cdef char *sscores

   cline = line
   ensurebufsize(strlen(cline)+1)
   strcpy(buf, cline)

   fields = split(buf, "|||", &n)

   # fix: check number of fields

   lhs = sym.fromstring(strip(fields[0]))
   fwords = fields[1]
   ewords = fields[2]
   sscores = fields[3]
   if n == 5:
      sattrs = fields[4]
   else:
      sattrs = None

   # for backwards compatibility
   cdef int n_scores
   scores = svector.Vector()
   fields = split(sscores, NULL, &n_scores)
   for i from 0 <= i < n_scores:
      cp = strrchr(fields[i], c'=')
      if cp != NULL:
         cp[0] = c'\0'
         scores[fields[i]] = strtof(cp+1, NULL)
      else:
         scores["pt%d" % i] = strtof(fields[i], NULL)

   attrs = {}
   if sattrs:
      si = 0
      m = attrs_re.match(sattrs, si)
      if m:
         while m:
            if m.group(3) is not None:
               attrs[m.group(1)] = m.group(3)
            else:
               attrs[m.group(1)] = m.group(4)
            si = m.end()
            m = attrs_re.match(sattrs, si)
      else:
         # default to align attr for compatibility
         attrs['align'] = sattrs

   # give align attr special treatment for compatibility
   if 'align' in attrs:
      align = []
      for a in attrs['align'].split():
         (ai, aj) = a.split("-")
         align.append((int(ai), int(aj)))
      if len(align) > 0:
         attrs['align'] = align
      else:
         del attrs['align']

   r = Rule(lhs, Phrase(fwords), Phrase(ewords), scores=scores, attrs=attrs)
      
   return r

def rule_copy(r):
   r1 = Rule(r.lhs, r.f, r.e, r.scores)
   r1.statelesscost = r.statelesscost
   r1.word_alignments = r.word_alignments
   return r1

