#!/usr/bin/env python

"""

Input:
  ... ||| frhs ||| ...

Output:
  sentence \t ... ||| frhs ||| ...

"""

# filter.py
# David Chiang <chiang@isi.edu>

import sys, os, os.path
import itertools

import time, math
import heapq

import simplerule

import log, monitor
log.level = 1

global_threshold = None

# rule filtering

import bisect
class Filter(object):
    def __init__(self, sents, maxlen):
        # substring -> set of sentence numbers
        self.all_substrs = {} # could replace with a suffix array for compactness
        # sentence number -> substring -> list of string positions
        self.substrs = [{} for sent in sents]
        self.n = [len(sent) for sent in sents]
        for sent_i in xrange(len(sents)):
            sent = sents[sent_i]
            m = maxlen or len(sent)
            for i in xrange(len(sent)):
                for j in xrange(i+1, min(i+m,len(sent))+1):
                    substr = tuple(sent[i:j])
                    self.all_substrs.setdefault(substr, set()).add(sent_i)
                    self.substrs[sent_i].setdefault(substr, []).append(i)
            for positions in self.substrs[sent_i].itervalues():
                positions.sort()

    @staticmethod
    def chunks(pattern):
        chunk = []
        for p in pattern:
            if isinstance(p, simplerule.Nonterminal):
                yield tuple(chunk)
                chunk = []
            else:
                chunk.append(p)
        yield tuple(chunk)

    def match1(self, pattern, substrs, n):
        """Returns True iff pattern matches the sentence whose substring index is substrs and length is n"""
        pos = 0
                
        for chunk in Filter.chunks(pattern):
            if len(chunk) == 0: # very common case
                if pos > n:
                    return False # even empty chunk can't match past end of sentence
                pos += 1 # +1 for hole filler
                continue
            if not substrs.has_key(chunk):
                return False
            positions = substrs[chunk]
            i = bisect.bisect_left(positions, pos)
            if i < len(positions):
                pos = positions[i]+len(chunk)+1 # +1 for hole filler
            else:
                return False
        return True

    def match(self, pattern):
        n_chunks = nonempty_chunks = 0
        for chunk in Filter.chunks(pattern):
            n_chunks += 1
            
            if len(chunk) == 0:
                continue

            try:
                sentlist = self.all_substrs[tuple(chunk)]
            except KeyError:
                return

            # try to avoid copying
            if nonempty_chunks == 0:
                matchsents = sentlist
            elif nonempty_chunks == 1:
                matchsents = matchsents & sentlist
            else:
                matchsents &= sentlist

            nonempty_chunks += 1

        if nonempty_chunks == 0: # nonlexical rule
            yield "global"
            return

        # If rule matches half or more, it's more worthwhile to
        # put it in the global grammar. This is a little overpermissive
        # because we haven't yet checked the order of chunk matches.
        if global_threshold is not None and len(matchsents) >= global_threshold*len(self.substrs):
            yield "global"
            return

        # if only one chunk, don't validate match.
        for sentnum in matchsents:
            if n_chunks == 1 or self.match1(pattern, self.substrs[sentnum], self.n[sentnum]):
                yield sentnum

if __name__ == "__main__":
    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option('-l', '--maxlen', dest='maxlen', type="int", default=None, help='maximum initial base phrase size')

    (opts,args) = optparser.parse_args()

    maxlen = opts.maxlen

    filterfilename = args[0]
    log.write("t=%s building filter from %s\n" % (monitor.cpu(), filterfilename))

    ffilter = Filter([line.split() for line in file(filterfilename)], maxlen=maxlen)

    log.write("t=%s begin filtering\n" % monitor.cpu())
    progress = 0
    for line in sys.stdin:
        #rule, _ = simplerule.Rule.from_str_hiero(line)
        #frhs = rule.frhs
        frhs = [simplerule.Nonterminal.from_str(f) for f in line.split(" ||| ", 3)[1].split()]
        for si in ffilter.match(frhs):
            print("%s\t%s" % (si,line.rstrip()))
        progress += 1
            
    
