#!/usr/bin/env python

# scorer.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

import sys, os, os.path

import monitor
import math
import heapq

import sym, rule, cost
import log
from filter import Filter
log.level = 1

PHRASE = sym.fromstring('[PHRASE]')

P_IMPROBABLE = 1e-7 # weights are recorded in our files to the 1e-6 place

def costfromprob(p):
    try:
        return -math.log10(p)
    except (ValueError, OverflowError):
        return cost.IMPOSSIBLE

profile = False

if not profile:
    try:
        import psyco
        psyco.profile()
    except ImportError:
        pass

if profile:
    import hotshot, hotshot.stats

### rule reading

def read_rules(files):
    """Merge several grammar files together (assuming they are sorted)."""

    if len(files) == 1:
        for line in files[0]:
            try:
                (handle, ruleline) = line.split("|||", 1)
                r = rule.rule_from_line(ruleline)
                yield handle, r
            except:
                sys.stderr.write("couldn't scan line: %s\n" % line.strip())
        return
    
    heap = []
    for f in files:
        try:
            line = f.next()
        except StopIteration:
            pass
        else:
            heap.append((line, f))
    heapq.heapify(heap)

    while len(heap) > 0:
        (line, f) = heapq.heappop(heap)

        try:
            (handle, ruleline) = line.split("|||", 1)
            r = rule.rule_from_line(ruleline)
        except:
            sys.stderr.write("couldn't scan line: %s\n" % line.strip())
            r = None

        if r is not None and len(r.scores) < 1:
            sys.stderr.write("rule doesn't have enough scores: %s\n" % str(r))
            r = None

        if r is not None:
            yield handle, r
            
        try:
            line = f.next()
        except StopIteration:
            pass
        else:
            heapq.heappush(heap, (line, f))

def read_rule_blocks(files):
    """Read all the rules with the same English side at a time, assuming
    that they are coming in English-major order."""
    block = None
    prev_handle = None
    for (handle, r) in read_rules(files):
        if prev_handle is not None and handle == prev_handle:
            block.append(r)
        else:
            if prev_handle is not None:
                yield block
            block = [r]
            prev_handle = handle
    if block is not None:
        yield block

interval = 1000000

def tabulate():
    if log.level >= 1:
        sys.stderr.write("(3) Tabulating filtered phrases\n")
    count = 1

    inputfiles = []
    for input in inputs:
        if os.path.isdir(input):
            inputfiles.extend(os.path.join(input, name) for name in os.listdir(input))
        else:
            inputfiles.append(input)
    inputfiles = [file(inputfile) for inputfile in inputfiles]

    global fsum, esum, allsum, xsum, gram
    fsum = {} # c(lhs, french)
    esum = {} # c(lhs, english)
    allsum = 0.0 # c(*)
    xsum = {} # c(lhs)
    gram = {}

    # read in all rules with matching english sides at the same time.
    # this way, we can sum only those english sides that ever appeared
    # with a french side that passes the filter.

    for rules in read_rule_blocks(inputfiles):
        flag = False
        blocksum = 0.
        for r in rules:
            scores = r.scores
            weight = scores[0]
            allsum += weight
            blocksum += weight
            xsum[r.lhs] = xsum.get(r.lhs, 0.0) + weight
            if ffilter is None or ffilter.match(r.f): # there used to be a shortcut here -- if fsum.has_key(r.f)
                #fsum[(r.lhs,r.f)] = fsum.get((r.lhs,r.f), 0.0) + weight
                fsum[r.f] = fsum.get(r.f, 0.0) + weight
                if r in gram:
                    gram[r] += r
                else:
                    gram[r] = r
                flag = True
            if log.level >= 1 and count%interval == 0:
                sys.stderr.write("time: %f, memory: %s, rules in: %d, rules counted: %d\n" % (monitor.cpu(), monitor.memory(), count, len(gram)))

            count += 1
        if flag:
            ewordsnorm = rules[0].e.handle()
            if ewordsnorm in esum:
                sys.stderr.write("warning: files not sorted properly\n")
            esum[ewordsnorm] = blocksum

def calculate():
    if log.level >= 1:
        sys.stderr.write("(4) Calculating probabilities\n")

    count = 1
    dropped = 0
    for r in gram.iterkeys():
        ewordsnorm = r.e.handle()
        scores = r.scores
        weight = scores[0]
        if weight <= opts.cutoff:
            dropped += 1
            continue
        try:
            newscores = [
                costfromprob(float(weight)/xsum[r.lhs]),             # p(e,f|x)
                #-math.log10(float(weight)/esum[(r.lhs, ewordsnorm)]), # p(f|e,x)
                costfromprob(float(weight)/esum[ewordsnorm]), # p(f|e)
                #-math.log10(float(weight)/fsum[(r.lhs, r.f)]),        # p(e|f,x)
                costfromprob(float(weight)/fsum[r.f]),        # p(e|f)
                #-math.log10(float(fsum[r.f])/allsum),          # p(f)
                #-math.log10(float(esum[ewordsnorm])/allsum),    # p(e)
                ]
            # the rest of the fields we find the weighted average of, using the first field as weight
            # fields 2 and 3 are interpreted as probabilities, the rest as costs. this is ugly
            if len(scores) >= 3:
                newscores.extend([
                    costfromprob(scores[1]/weight),             # lexical weight
                    costfromprob(scores[2]/weight),             # lexical weight
                ])
                # anything else
                newscores.extend([score/weight for score in scores[3:]])
            r.scores = newscores
            output_file.write("%s\n" % r.to_line())
        except (OverflowError, ZeroDivisionError, KeyError):
            sys.stderr.write("warning: division by zero or log of zero: %s, xsum=%s fsum=%s esum=%s allsum=%s\n" % (r.to_line(), xsum[r.lhs], fsum[r.f], esum[ewordsnorm], allsum))
        if log.level >= 1 and count%interval == 0:
            sys.stderr.write("time: %f, rules out: %d, dropped: %d\n" % (monitor.cpu(), count, dropped))
        count += 1

    # obsolete
    """for (x,s) in xsum.iteritems():
        if x != PHRASE:
            sys.stderr.write("output PHRASE -> %s\n" % sym.tostring(x))
            # or should it be relative to PHRASE?
            x = sym.setindex(x, 1)
            try:
                r = rule.Rule(PHRASE, rule.Phrase([x]), rule.Phrase([x]), scores=[-math.log10(float(s)/allsum), 0.0, 0.0, 0.0, 0.0])
            except OverflowError:
                sys.stderr.write("warning: overflow error: x=%s, xsum=%s, allsum=%s\n" % (x, s, allsum))
            output_file.write("%s\n" % r.to_line())"""
    log.write("%d dropped total\n" % dropped)

if __name__ == "__main__":
    import gc
    gc.set_threshold(100000,10,10) # this makes a huge speed difference
    #gc.set_debug(gc.DEBUG_STATS)

    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option("-f", "--filter", dest="filter_file", help="test file to filter for")
    optparser.add_option('-L', '--maxabslen', dest='maxabslen', type="int", default=12, help='maximum initial base phrase size')
    optparser.add_option('-l', '--maxlen', dest='maxlen', type="int", default=5, help='maximum final phrase size')
    optparser.add_option('-c', '--cutoff', dest='cutoff', type="float", default=0.0, help='prune rules with count less than or equal to parameter')
    optparser.add_option('-p', '--parallel', dest="parallel", nargs=2, type="int", help="only use i'th sentence out of every group of j in filter file")
    (opts,args) = optparser.parse_args()

    maxlen = opts.maxlen
    maxabslen = opts.maxabslen

    output_file = sys.stdout

    inputs = args

    if opts.filter_file is not None:
        lines = [[sym.fromstring(w) for w in line.split()] for line in file(opts.filter_file)]

        if opts.parallel is not None:
            i,j = opts.parallel
            lines = lines[i::j]

        ffilter = Filter(lines, maxlen)
        log.write("Filtering using %s\n" % opts.filter_file)
    else:
        ffilter = None

    if profile:
        prof = hotshot.Profile("scorer.prof")
        prof.start()

    tabulate()
    del ffilter
    calculate()

    if profile:
        prof.stop()
        prof.close()
        stats = hotshot.stats.load("scorer.prof")
        stats.strip_dirs()
        stats.sort_stats('time', 'calls')
        stats.print_stats(100)
        
    log.write("\nDone\n")

