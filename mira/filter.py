#!/usr/bin/env python

# filter.py
# David Chiang <chiang@isi.edu>

import sys, os, os.path
import itertools

import time, math
import heapq

import sym, rule
import log
log.level = 1

PHRASE = sym.fromstring('[PHRASE]')

P_IMPROBABLE = 1e-7 # weights are recorded in our files to the 1e-6 place

profile = False

if not profile:
    try:
        import psyco
        psyco.profile()
    except ImportError:
        pass

if profile:
    import hotshot, hotshot.stats

# rule filtering

class NewFilter(object):
    def __init__(self, sents, maxlen):
        self.sents = sents
        self.suffixes = [(sent_i, word_i) for sent_i in xrange(len(sents)) for word_i in xrange(len(sents[sent_i]))]

    def compare_suffix((si1, wi1), (si2, wi2)):
        n1 = len(self.sents[si1])
        n2 = len(self.sents[si2])

        k = 0
        
        while wi1 + k < n1 and wi2 + k < n2:
            c1 = self.sents[si1][wi1+k]
            c2 = self.sents[si2][wi2+k]
            if c1 < c2:
                return -1
            elif c1 > c2:
                return +1
            
import bisect
class Filter(object):
    def __init__(self, sents, maxlen):
        self.all_substrs = {} # could replace with a suffix array for compactness
        self.substrs = [{} for sent in sents]
        self.n = [len(sent) for sent in sents]
        self.maxlen = maxlen
        for sent_i in xrange(len(sents)):
            sent = sents[sent_i]
            for i in xrange(len(sent)):
                for j in xrange(i+1, min(i+maxlen,len(sent))+1):
                    self.all_substrs.setdefault(tuple(sent[i:j]), set()).add(sent_i)
                    #self.all_substrs.setdefault(tuple(sent[i:j]), []).append(sent_i)
                    self.substrs[sent_i].setdefault(tuple(sent[i:j]), []).append(i)
            for positions in self.substrs[sent_i].itervalues():
                positions.sort()

    def match1(self, pattern, substrs, n):
        """Returns True iff pattern matches the sentence whose substring index is substrs and length is n"""
        pos = 0
        for ci in xrange(pattern.arity()+1):
            chunk = tuple(pattern.getchunk(ci))
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
        if pattern.arity() == 0: # very common case
            return self.all_substrs.has_key(tuple(pattern.getchunk(0)))

        if len(self.substrs) == 1: # common case
            return self.match1(pattern, self.substrs[0], self.n[0])

        matchsents = None
        n_chunks = 0
        
        for ci in xrange(pattern.arity()+1):
            chunk = pattern.getchunk(ci)
            if len(chunk) == 0:
                continue

            try:
                sentlist = self.all_substrs[tuple(chunk)]
            except KeyError:
                return False

            if n_chunks == 0:
                matchsents = sentlist
            elif n_chunks == 1:
                matchsents = matchsents & sentlist
            else:
                matchsents &= sentlist

            n_chunks += 1

        if matchsents is None:
            return True # nonlexical rule

        for sentnum in matchsents:
            if self.match1(pattern, self.substrs[sentnum], self.n[sentnum]):
                return True
        return False

if __name__ == "__main__":
    import gc
    gc.set_threshold(100000,10,10) # this makes a huge speed difference
    #gc.set_debug(gc.DEBUG_STATS)

    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option("-f", "--filter", dest="filter_file", help="test file to filter for")
    optparser.add_option('-L', '--maxabslen', dest='maxabslen', type="int", default=12, help='maximum initial base phrase size')
    optparser.add_option('-l', '--maxlen', dest='maxlen', type="int", default=5, help='maximum final phrase size')
    optparser.add_option('-p', '--parallel', dest="parallel", nargs=2, type="int", help="only use i'th sentence out of every group of j in filter file")
    optparser.add_option('-b', '--begin', dest="begin", type="int", help="number to start numbering sentence grammars at", default=0)
    optparser.add_option('-s', '--sentence-grammars', dest="sentence_grammars", help="produce per-sentence grammars in specified directory")

    (opts,args) = optparser.parse_args()

    maxlen = opts.maxlen
    maxabslen = opts.maxabslen

    if opts.sentence_grammars is not None:
        try:
            output_dir = opts.sentence_grammars
            os.mkdir(output_dir)
        except OSError:
            sys.stderr.write("warning: directory already exists\n")
    else:
        output_file = sys.stdout

    import fileinput

    if opts.filter_file is None:
        raise Exception("You must specify a dataset to filter for.")

    lines = [[sym.fromstring(w) for w in line.split()] for line in file(opts.filter_file)]

    if opts.sentence_grammars:
        lines = list(enumerate(lines))

    if opts.parallel is not None:
        i,j = opts.parallel
        lines = lines[i::j]

    if opts.sentence_grammars:
        ffilters = [(Filter([line], maxlen), file(os.path.join(output_dir, "grammar.line%d" % (i+opts.begin)), "w")) for (i,line) in lines]
    else:
        ffilters = [(Filter(lines, maxlen), output_file)]

    progress = 0
    for line in fileinput.input(args):
        r = rule.rule_from_line(line)
        for (ffilter, output_file) in ffilters:
            if ffilter.match(r.f):
                output_file.write(line)
        progress += 1
        if progress % 100000 == 0:
            sys.stderr.write("%d rules in\n" % progress)
            
    
