#!/usr/bin/env python

# lexweights.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

# should a word with multiple alignments evenly distribute 1 point among its partners?

"""fweightfile (Pharaoh lex.n2f) is in the format
f e P(f|e)
eweightfile (Pharaoh lex.f2n) is in the format
e f P(e|f)
where either e or f can be NULL.

ratiofile is in eweightfile format.
"""

import sys, math, itertools
import alignment, sym
import extractor
import log

def read_weightfile(f, threshold=None):
    w = {}
    progress = 0
    log.write("Reading ttable from %s..." % f.name)
    for line in f:
        progress += 1
        if progress % 100000 == 0:
            log.write(".")
        (word1, word2, p) = line.split()
        p = float(p)
        if threshold is not None and p < threshold:
            continue
        if word1 == "NULL":
            word1 = None
        else:
            word1 = sym.fromstring(word1)
        if word2 == "NULL":
            word2 = None
        else:
            word2 = sym.fromstring(word2)
        w.setdefault(word1,{}).setdefault(word2, p)
    log.write("done\n")
    return w

def compute_weights(a, w, transpose=False, swap=False, threshold=1e-40):
    """
    transpose: compute weights for English instead of French
    swap: file format has first two columns swapped
    """
    result = []
    if not transpose:
        fwords,ewords,faligned = a.fwords, a.ewords, a.faligned
    else:
        fwords,ewords,faligned = a.ewords, a.fwords, a.ealigned
    for i in xrange(len(fwords)):
        total = 0.0
        n = 0
        if faligned[i]:
            for j in xrange(len(ewords)):
                if not transpose:
                    flag = a.aligned[i][j]
                else:
                    flag = a.aligned[j][i]
                if flag:
                    try:
                        if not swap:
                            total += w[fwords[i]][ewords[j]]
                        else:
                            total += w[ewords[j]][fwords[i]]
                    except:
                        #log.write("warning: couldn't look up lexical weight for (%s,%s)\n" % (sym.tostring(fwords[i]), sym.tostring(ewords[j])))
                        if threshold is not None:
                            total += threshold
                        
                    n += 1
        else:
            try:
                if not swap:
                    total += w[fwords[i]][None]
                else:
                    total += w[None][fwords[i]]
            except:
                #log.write("warning: couldn't look up null alignment for %s\n" % sym.tostring(fwords[i]))
                if threshold is not None:
                    total += threshold

            n += 1
        result.append(float(total)/n)
    return result

class LexicalWeightFeature(extractor.Feature):
    def __init__(self, fweightfile, eweightfile, ratiofile, threshold=None):
        self.threshold = threshold
        if fweightfile is not None:
            self.fweighttable = read_weightfile(file(fweightfile), threshold=threshold)
        else:
            self.fweighttable = None
            
        if eweightfile is not None:
            self.eweighttable = read_weightfile(file(eweightfile), threshold=threshold)
        else:
            self.eweighttable = None

        if ratiofile is not None:
            self.ratiotable = read_weightfile(file(opts.ratiofile))
            for ((word1,word2),p) in self.ratiotable.iteritems():
                # this cutoff determined by looking at ratio vs. rank
                # basically the curve has three parts:
                if p > 100.0: 
                    self.ratiotable[word1,word2] = 1.0
                elif p > 1.0: # long flat region
                    self.ratiotable[word1,word2] = 0.5
                else:
                    self.ratiotable[word1,word2] = 0.0
        else:
            self.ratiotable = None

    def process_alignment(self, a):
        self.fweights = self.eweights = self.fratios = None
        if self.fweighttable is not None:
            self.fweights = compute_weights(a, self.fweighttable, threshold=self.threshold)
        if self.eweighttable is not None:
            self.eweights = compute_weights(a, self.eweighttable, transpose=True, threshold=self.threshold)
        if self.ratiotable is not None:
            self.fratios = compute_weights(a, self.ratiotable, swap=True)
    
    def score_rule(self, a, r):
        fweight = eweight = 1.0
        fratio = 0.0
        
        for i in xrange(len(r.f)):
            if not sym.isvar(r.f[i]):
                if self.fweights is not None:
                    fweight *= self.fweights[r.fpos[i]]
                if self.fratios is not None:
                    fratio += self.fratios[r.fpos[i]]

        for i in xrange(len(r.e)):
            if not sym.isvar(r.e[i]):
                if self.eweights is not None:
                    eweight *= self.eweights[r.epos[i]]

        scores = []
        if self.fweights is not None:
            scores.append(fweight)
        if self.eweights is not None:
            scores.append(eweight)
        if self.fratios is not None:
            scores.append(fratio)

        return scores

# for calculating likelihood ratios. Manning and Schuetze p. 173
# note natural logs
def ll(k,n,x):
    if k == 0:
        return n*math.log(1-x)
    elif n-k == 0:
        return n*math.log(x)
    else:
        return k*math.log(x) + (n-k)*math.log(1-x)

def llr(n,c1,c2,c12):
    p = float(c2)/n
    p1 = float(c12)/c1
    p2 = float(c2-c12)/(n-c1)
        
    return ll(c12,c1,p) + ll(c2-c12,n-c1,p) - ll(c12,c1,p1) - ll(c2-c12,n-c1,p2)

if __name__ == "__main__":
    import rule

    threshold = 1e-8
    fweightfile = sys.argv[1]
    eweightfile = sys.argv[2]

    fweighttable = read_weightfile(file(fweightfile), threshold=threshold)
    eweighttable = read_weightfile(file(eweightfile), threshold=threshold)

    progress = 0
    for line in sys.stdin:
        r = rule.rule_from_line(line)
        if r.word_alignments is None:
            scores = r.scores
            scores.extend([scores[0],scores[0]])
            r.scores = scores
            sys.stdout.write("%s\n" % r.to_line())
            progress += 1
            continue
        
        align = set(r.word_alignments)

        fweight = eweight = 1.0
        
        for fi in xrange(len(r.f)):
            if not sym.isvar(r.f[fi]):
                fwordweight = 0.
                n = 0
                for ei in xrange(len(r.e)):
                    if (fi, ei) in align:
                        try:
                            fwordweight += fweighttable[r.f[fi]][r.e[ei]]
                        except KeyError:
                            fwordweight += threshold
                        n += 1
                if n > 0:
                    fweight *= fwordweight / n
                else:
                    try:
                        fweight = fweighttable[r.f[fi]][None]
                    except KeyError:
                        fweight = threshold

        for ei in xrange(len(r.e)):
            if not sym.isvar(r.e[ei]):
                ewordweight = 0.
                n = 0
                for fi in xrange(len(r.f)):
                    if (fi, ei) in align:
                        try:
                            ewordweight += eweighttable[r.e[ei]][r.f[fi]]
                        except KeyError:
                            ewordweight += threshold
                        n += 1
                if n > 0:
                    eweight *= ewordweight / n
                else:
                    try:
                        eweight = eweighttable[r.e[ei]][None]
                    except KeyError:
                        eweight = threshold

        scores = r.scores
        scores.extend([fweight*scores[0], eweight*scores[0]])
        r.scores = scores

        sys.stdout.write("%s\n" % r.to_line())
        progress += 1

    sys.stderr.write("Done (%d rules)\n" % progress)

# this code is no longer used
if False: # __name__ == "__main__":
    try:
        import psyco
        psyco.full()
    except ImportError:
        pass
    
    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option('-w', '--weight', nargs=2, dest='weightfiles', help="lexical weight tables")
    optparser.add_option('-W', '--words', nargs=2, dest='words', help="parallel text files (words)")
    optparser.add_option('-P', '--pharaoh', dest='pharaoh', action='store_true', default=False, help="input is Pharaoh-style alignment (requires -W)")
    optparser.add_option('-r', '--ratio', dest='ratiofile', help="likelihood ratio file")
    (opts,args) = optparser.parse_args()

    if opts.words is not None:
        ffilename, efilename = opts.words
        ffile = file(ffilename)
        efile = file(efilename)

    if len(args) == 0:
        args = ["-"]

    if opts.pharaoh:
        if len(args) != 1:
            sys.stderr.write("Can only read in one file in Pharaoh format\n")
        if opts.words is None:
            sys.stderr.write("-W option required for Pharaoh format\n")
        if args[0] == "-":
            input_file = sys.stdin
        else:
            input_file = file(args[0], "r")
        alignments = alignment.Alignment.reader_pharaoh(ffile, efile, input_file)
    else:
        input_files = []
        for arg in args:
            if arg == "-":
                input_files.append(sys.stdin)
            else:
                input_files.append(file(args[0], "r"))
        alignments = itertools.chain(*[alignment.Alignment.reader(input_file) for input_file in input_files])
        # bug: ignores -W option

    if opts.weightfiles is not None:
        fweightfile = file(opts.weightfiles[0], "w")
        eweightfile = file(opts.weightfiles[1], "w")

    if opts.ratiofile is not None:
        ratiofile = file(opts.ratiofile, "w")
        
    fcount = {}
    ecount = {}
    fecount = {}
    count = 0


    progress = 0
    for a in alignments:
        null = sym.fromstring("NULL")
        # Calculate lexical weights
        for i in xrange(len(a.fwords)):
            for j in xrange(len(a.ewords)):
                if a.aligned[i][j]:
                    count += 1
                    fcount[a.fwords[i]] = fcount.get(a.fwords[i],0)+1
                    ecount[a.ewords[j]] = ecount.get(a.ewords[j],0)+1
                    fecount[(a.fwords[i],a.ewords[j])] = fecount.get((a.fwords[i],a.ewords[j]),0)+1

        for i in xrange(len(a.fwords)):
            if not a.faligned[i]:
                count += 1
                fcount[a.fwords[i]] = fcount.get(a.fwords[i],0)+1
                ecount[null] = ecount.get(null,0)+1
                fecount[(a.fwords[i],null)] = fecount.get((a.fwords[i],null),0)+1
        for j in xrange(len(a.ewords)):
            if not a.ealigned[j]:
                count += 1
                fcount[null] = fcount.get(null,0)+1
                ecount[a.ewords[j]] = ecount.get(a.ewords[j],0)+1
                fecount[(null,a.ewords[j])] = fecount.get((null,a.ewords[j]),0)+1

        progress += 1
        if progress % 10000 == 0:
            sys.stderr.write(".")

    # Dump lexical weights
    for (fword,eword) in fecount.keys():
        if opts.ratiofile:
            # f|e
            c12 = fecount[fword,eword]
            c1 = ecount[eword]
            c2 = fcount[fword]
            p = float(c2)/count
            p1 = float(c12)/c1
            p2 = float(c2-c12)/(count-c1)
            ratiofile.write("%s %s %f\n" % (sym.tostring(eword), sym.tostring(fword), -2*llr(count,ecount[eword],fcount[fword],fecount[fword,eword])))
        if opts.weightfiles:
            fweightfile.write("%s %s %f\n" % (sym.tostring(fword), sym.tostring(eword), float(fecount[(fword,eword)])/ecount[eword]))
            eweightfile.write("%s %s %f\n" % (sym.tostring(eword), sym.tostring(fword), float(fecount[(fword,eword)])/fcount[fword]))

    sys.stderr.write("\n")
