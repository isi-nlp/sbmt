#!/usr/bin/env python

# extractor.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

"""
Input:
  french \t english \t f-e f-e f-e
where f and e are 0-based positions in french and english string

Output:
  [LHS] ||| frhs ||| erhs ||| align={{{f-e f-e f-e}}} \t count=c

The output is not required to be sorted or uniqed.
"""

"""
Future ideas:
  - change notion of "tight" so that outer phrases are minimal and inner phrases are maximal?


would this be a faster criterion?
keep track of number of alignments in each f/e span
for each f span, find max/min aligned e words
if number of links in f span is equal to number of links in e span, then it's a phrase

"""

import sys
import collections, itertools, bisect
import tree
import alignment
import log, monitor
import simplerule
import svector

log.level = 1

combiner = True

PHRASE = simplerule.Nonterminal('PHRASE')
START = simplerule.Nonterminal('START')
nonterminals = [PHRASE]

punctuation = "PU PUNC . , : `` '' -LRB- -RRB-".split()

def extract_phrases(self, maxlen):
    ifirst = [len(self.fwords) for j in self.ewords]
    ilast = [0 for j in self.ewords]
    jfirst = [len(self.ewords) for i in self.fwords]
    jlast = [0 for i in self.fwords]
    for i in xrange(len(self.fwords)):
        for j in xrange(len(self.ewords)):
            if self.aligned[i][j]:
                if j<jfirst[i]:
                    jfirst[i] = j
                jlast[i] = j+1
                if i<ifirst[j]:
                    ifirst[j] = i
                ilast[j] = i+1

    for i1 in xrange(len(self.fwords)):
        if not self.faligned[i1]:
            continue
        j1 = len(self.ewords)
        j2 = 0
        for i2 in xrange(i1+1,min(len(self.fwords),i1+maxlen)+1):
            if not self.faligned[i2-1]:
                continue
            # find biggest empty left and right blocks
            j1 = min(j1, jfirst[i2-1])
            j2 = max(j2, jlast[i2-1])

            # make sure band isn't empty
            if j1 >= j2:
                continue

            # check minimum top and bottom blocks
            if j2-j1 > maxlen:
                break # goto next i1 value

            next = 0
            for j in xrange(j1, j2):
                if ifirst[j]<i1:
                    next = 1
                    break
                if ilast[j]>i2:
                    next = 2
                    break
            if next == 1:
                break # goto next i1 value
            elif next == 2:
                continue # goto next i2 value

            yield((i1,j1,i2,j2))

def loosen_phrases(self, phrases, maxlen, french_loose_limit=None, english_loose_limit=None):
    for (i1_max,j1_max,i2_min,j2_min) in phrases:
        i1_min = i1_max
        stop = 0 if french_loose_limit is None else max(0,i1_min-french_loose_limit)
        while i1_min > stop and self.faligned[i1_min-1] == 0:
            i1_min -= 1

        j1_min = j1_max
        stop = 0 if english_loose_limit is None else max(0,j1_min-english_loose_limit)
        while j1_min > stop and self.ealigned[j1_min-1] == 0:
            j1_min -= 1

        i2_max = i2_min
        stop = len(self.fwords) if french_loose_limit is None else min(len(self.fwords),i2_max+french_loose_limit)
        while i2_max < stop and self.faligned[i2_max] == 0:
            i2_max += 1

        j2_max = j2_min
        stop = len(self.ewords) if english_loose_limit is None else min(len(self.ewords),j2_max+english_loose_limit)
        while j2_max < stop and self.ealigned[j2_max] == 0:
            j2_max += 1

        for i1 in xrange(i1_min, i1_max+1):
            for i2 in xrange(max(i1+1,i2_min), min(i2_max,i1+maxlen)+1):
                for j1 in xrange(j1_min, j1_max+1):
                    for j2 in xrange(max(j1+1,j2_min), min(j2_max,j1+maxlen)+1):
                        yield (i1,j1,i2,j2)

def crossing_brackets(t):
    cb = collections.defaultdict(int)
    for ni,nj in t.nodes_byspan():
        for nk in xrange(ni+1,nj):
            for nl in xrange(ni):
                cb[nl,nk] += 1
            for nl in xrange(nj+1,t.length+1):
                cb[nk,nl] += 1
    return cb

def tree_fragment(t, i, j):
    if not hasattr(t, "spans"):
        n = t.length
        t.spans = {}
        t.iindex = [[] for i1 in xrange(n+1)]
        t.jindex = [[] for i1 in xrange(n+1)]
        j1 = 0
        for node in t.postorder():
            if node.is_terminal():
                t.spans[node] = (j1,j1+1)
                j1 += 1
            else:
                i1 = t.spans[node.children[0]][0]
                t.spans[node] = (i1,j1)
                t.iindex[i1].append(node)
                t.jindex[j1].append(node)

    label = []
    k = i
    while k < j:
        # find highest node starting at k that is within (i,j)
        for node in reversed(t.iindex[k]):
            ni, nj = t.spans[node]
            if i <= ni and nj <= j:
                break
        else:
            assert False

        label.append('(%s)' % node.label)

        k = t.spans[node][-1]
        if k < j:
            # find nodes that are incomplete at k
            for node in t.jindex[k]:
                ni, nj = t.spans[node]
                if ni < i:
                    label.append('%s)' % node.label)
            for node in t.iindex[k]:
                ni, nj = t.spans[node]
                if j < nj:
                    label.append('(%s' % node.label)
    return "".join(label)

def filter_phrase(a, (fi, fj, ei, ej)):
    if opts.french_trees and opts.french_crossing_brackets is not None:
        return fcb[fi, fj] <= opts.french_crossing_brackets
    elif opts.english_trees and opts.english_crossing_brackets is not None:
        return ecb[ei, ej] <= opts.english_crossing_brackets
    else:
        return True

def label_phrase(a, (fi,fj,ei,ej)):
    if opts.french_trees and opts.english_trees:
        fx = tree_fragment(ftree, fi, fj)
        ex = tree_fragment(etree, ei, ej)
        return [simplerule.Nonterminal("%s:%s" % (fx, ex))]
    # to do: tree->string or string->tree
    else:
        return [PHRASE]

def subtract_phrases(self, phrases, maxlen, minhole, maxvars):
    if type(phrases) is not list:
        phrases = list(phrases)
    if len(phrases) == 0:
        log.write("warning: no phrases extracted\n")
        return []
    maxabslen = max([i2-i1 for (x,i1,j1,i2,j2) in phrases])

    # Search space can be thought of as a directed graph where the
    # vertices are between-symbol positions, and the edges are
    # words or phrases. Our goal is to find all paths in the graph
    # which start at the left edge of a phrase and end at the right edge.

    # Find all paths of length maxlen or less starting from the left
    # edge of a phrase

    n = len(self.fwords)
    chart = [[[[] for nvars in xrange(maxvars+1)] for i2 in xrange(n+1)] for i1 in xrange(n+1)]
    i2index = [[] for i2 in xrange(n+1)]
    i1s = set()

    for phrase in phrases:
        (x,i1,j1,i2,j2) = phrase
        i2index[i2].append(phrase)
        i1s.add(i1)

    for i in i1s:
        chart[i][i][0].append([])
    for k in xrange(1,min(maxabslen,n)+1):
        for i1 in [i for i in i1s if i+k<=n]:
            i2 = i1+k
            # find a subphrase
            if maxvars > 0:
                for subphrase in i2index[i2]:
                    (sub_x, sub_i1, sub_j1, sub_i2, sub_j2) = subphrase
                    if sub_i2-sub_i1 >= minhole: # or sub_x is not PHRASE:
                        # not very efficient because no structure sharing
                        for nvars in xrange(maxvars):
                            for item in chart[i1][sub_i1][nvars]:
                                if (len(item) < maxlen
                                    and not (opts.forbid_adjacent and len(item)>0 and type(item[-1]) is not int)
                                    ):
                                        chart[i1][i2][nvars+1].append(item + [subphrase])
            # find a word
            for nvars in xrange(maxvars+1):
                for item in chart[i1][i2-1][nvars]:
                    if len(item) < maxlen:
                        chart[i1][i2][nvars].append(item + [i2-1])

    # Now for each phrase, find all paths starting from left edge and
    # ending at right edge
    wholeresult = []
    for (x,i1,j1,i2,j2) in phrases:
        result = []
        for nvars in xrange(maxvars+1):
            for fwords in chart[i1][i2][nvars]:
                r = make_rule(self, (x,i1,j1,i2,j2), fwords)
                if r is not None:
                    r.scores['count'] = 1.
                    result.append(r)

        # Normalize
        if not opts.uniform_counts:
            for r in result:
                r.scores /= len(result)
        wholeresult.extend(result)


    return wholeresult

pseudoroots = ['TOP','','TOP-BAR','TOP-BAR{}'] # first one is the canonical one
def prepare_tree(t):
    if t is None or t.label == '0' and t.is_terminal():
        return None

    # remove pseudoroots
    while t.label in pseudoroots and len(t.children) == 1:
        t = t.children[0]
    t.detach()
    # but if pseudoroot had more than one child, we couldn't remove it, so canonicalize it
    if t.label in pseudoroots:
        t.label = pseudoroots[0]

    i = 0
    for node in t.postorder():
        if node.is_terminal():
            node.span = (i,i+1)
            i += 1
        else:
            node.span = (node.children[0].span[0], node.children[-1].span[1])

    return t

def make_rule(self, (x,i1,j1,i2,j2), fwords):
    '''fwords is a list of numbers and subphrases:
       the numbers are indices into the French sentence'''
    # omit trivial rules
    if len(fwords) == 1 and type(fwords[0]) is not int:
        return None

    fwords = fwords[:]
    fpos = [None for w in fwords]
    ewords = self.ewords[j1:j2]
    elen = j2-j1
    fweight = eweight = 1.0
    funaligned = eunaligned = 0
    index = 1
    flag = False
    for i in xrange(len(fwords)):
        fi = fwords[i]
        if type(fi) is int: # terminal symbol
            if self.faligned[fi]:
                flag = True
            fwords[i] = self.fwords[fi]
            fpos[i] = fi
        else: # nonterminal symbol
            (sub_x,sub_i1,sub_j1,sub_i2,sub_j2) = fi
            sub_j1 -= j1
            sub_j2 -= j1
            
            # Check English holes
            # can't lie outside phrase
            if sub_j1 < 0 or sub_j2 > elen:
                return None

            # can't overlap
            for j in xrange(sub_j1,sub_j2):
                if type(ewords[j]) is tuple or ewords[j] is None:
                    return None

            # Set first eword to var, rest to None

            # We'll clean up the Nones later
            v = sub_x.setindex(index)
            fwords[i] = v
            fpos[i] = (sub_i1,sub_i2)
            ewords[sub_j1] = (v, sub_j1+j1, sub_j2+j1)
            for j in xrange(sub_j1+1,sub_j2):
                ewords[j] = None
            index += 1

    # Require an aligned French word
    if opts.require_aligned_terminal and not flag:
        return None

    epos = []
    new_ewords = []
    for i in xrange(elen):
        if ewords[i] is not None:
            if type(ewords[i]) is tuple:
                (v, ei, ej) = ewords[i]
                new_ewords.append(v)
                epos.append((ei,ej))
            else:
                new_ewords.append(ewords[i])
                epos.append(i+j1)
    ewords = new_ewords
                
    r = simplerule.Rule(x,fwords,ewords)
    r.fpos = fpos
    r.epos = epos
    r.span = (i1,i2,j1,j2)

    if opts.keep_word_alignments:
        align = []
        for fi in xrange(len(fpos)):
            if type(fpos[fi]) is int:
                for ei in xrange(len(epos)):
                    if type(epos[ei]) is int:
                        if a.aligned[fpos[fi]][epos[ei]]:
                            align.append((fi,ei))
        r.attrs['align'] = " ".join("%d-%d" % (fi,ei) for (fi,ei) in align)

    return r

if __name__ == "__main__":
    import gc
    gc.set_threshold(100000,10,10) # this makes a huge speed difference
    #gc.set_debug(gc.DEBUG_STATS)

    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option('-L', '--maxabslen', dest='maxabslen', type="int", default=None, help='maximum initial base phrase size')
    optparser.add_option('-l', '--maxlen', dest='maxlen', type="int", default=5, help='maximum final phrase size')
    optparser.add_option('-s', '--minsublen', dest='minhole', type="int", default=1, help='minimum subphrase size')
    optparser.add_option('-v', '--maxvars', dest='maxvars', type="int", default=2, help='maximum number of nonterminals')
    optparser.add_option('--minvars', dest='minvars', type="int", default=0, help='minimum number of nonterminals')
    optparser.add_option('-t', '--tight', dest='loosen', action='store_false', default=True, help='use tight phrases only')
    optparser.add_option('--english-loose-limit', dest='english_loose_limit', type="int")
    optparser.add_option('--french-loose-limit', dest='french_loose_limit', type="int")
    optparser.add_option('-a', '--allow-adjacent', dest="forbid_adjacent", action="store_false", default=True, help="allow adjacent nonterminals")
    optparser.add_option('-n', '--allow-nonlexical', dest="require_aligned_terminal", action="store_false", default=True, help="allow nonlexical rules")
    optparser.add_option('-A', '--keep-word-alignments', dest="keep_word_alignments", help="keep word alignments", action="store_true", default=False)
    optparser.add_option('-D', '--discard-long-sentences', dest="discard_long_sentences", action="store_true", default=False, help="discard sentences longer than -L")
    optparser.add_option('-u', '--uniform-counts', dest="uniform_counts", action="store_true", default=False)
    optparser.add_option('--french-trees', dest="french_trees", action="store_true", default=False)
    optparser.add_option('--english-trees', dest="english_trees", action="store_true", default=False)
    optparser.add_option('--french-crossing-brackets', dest="french_crossing_brackets", type=int, default=None)
    optparser.add_option('--english-crossing-brackets', dest="english_crossing_brackets", type=int, default=None)

    (opts,args) = optparser.parse_args()

    maxlen = opts.maxlen
    maxabslen = opts.maxabslen

    # for phrase-only extraction, we can optimize
    if opts.maxvars == 0:
        if not maxabslen or maxabslen > maxlen:
            maxabslen = maxlen
        if maxabslen and maxlen > maxabslen:
            maxlen = maxabslen

    gram = {}

    for li, line in enumerate(sys.stdin):
        fields = line.rstrip().split('\t')
        try:
            fstr, estr, astr = fields[:3]
        except:
            log.write("bad line, skipped: %s\n" % line.rstrip('\r\n'))
            continue
        
        if len(fields) == 4:
            provenance = list(fields[3].split())
        else:
            provenance = []

        if opts.french_trees:
            try:
                ftree = prepare_tree(tree.str_to_tree(fstr))
                if ftree is None:
                    log.write("bad tree, skipped line\n")
                    continue

                #fspans = label_spans(ftree)
                fleaves = list(ftree.frontier())
                fwords = [leaf.label for leaf in fleaves]
                ftags = [leaf.parent.label for leaf in fleaves]
                fcb = crossing_brackets(ftree)
            except Exception as e:
                log.write("bad ftree, skipped: %s\n" % fstr)
                log.write("reason: %s\n" % e)
                continue
        else:
            fwords = fstr.split()

        if opts.english_trees:
            try:
                etree = prepare_tree(tree.str_to_tree(estr))
                if etree is None:
                    log.write("bad tree, skipped line\n")
                    continue

                #espans = label_spans(etree)
                eleaves = list(etree.frontier())
                ewords = [leaf.label for leaf in eleaves]
                etags = [leaf.parent.label for leaf in eleaves]
                ecb = crossing_brackets(etree)
            except:
                log.write("bad etree, skipped: %s\n" % estr)
                continue
        else:
            ewords = estr.split()
            
        a = alignment.Alignment(fwords, ewords)
        good = True
        if astr.strip() == "":
            log.write("skipping sentence with empty alignment\n")
            continue
        for ij in astr.split():
            i,j = (int(x) for x in ij.split('-',1))
            try:
                a.align(i,j)
            except IndexError:
                log.write("alignment point %s-%s out of bounds\n" % (i,j))
                good = False
                break
        if not good:
            log.write("french:    %s\n" % " ".join(fwords))
            log.write("english:   %s\n" % " ".join(ewords))
            log.write("alignment: %s\n" % astr)
            continue
                
            
        if log.level >= 2:
            a.write(log.file)
            a.write_visual(log.file)

        # done reading all input lines
        if opts.discard_long_sentences and opts.maxabslen and len(a.fwords) > opts.maxabslen:
            continue

        phrases = extract_phrases(a, maxabslen or max(len(fwords),len(ewords)))

        if opts.loosen:
            phrases = loosen_phrases(a, phrases,
                                     maxabslen or max(len(fwords),len(ewords)),
                                     opts.french_loose_limit, opts.english_loose_limit)

        labeled = []
        for (fi,ei,fj,ej) in phrases:
            if not filter_phrase(a, (fi, fj, ei, ej)): continue
            labels = label_phrase(a, (fi,fj,ei,ej))
            labeled.extend([(x,fi,ei,fj,ej) for x in labels])
        phrases = labeled

        rules = subtract_phrases(a, phrases, maxlen, opts.minhole, opts.maxvars)

        if opts.minvars:
            rules = [r for r in rules if r.arity() >= opts.minvars]

        if opts.uniform_counts:
            # squash counts to 1
            squashed = set(rules)
            for r in squashed:
                r.scores['count'] = 1
            rules = squashed

        # add provenance features
        for r in rules:
            for p in provenance:
                r.scores['prov_%s' % p] = 1

        if len(rules) >= 100000:
            log.write("sentence %s has %s rules\n" % (li+1, len(rules)))
            log.write("input: %s\n" % line.rstrip())

        if not combiner:
            # simple version
            for r in rules:
                print "%s\t%s" % (r, r.scores)
        else:
            # do some combining before writing out
            for r in rules:
                existing = gram.get(r, None)
                if existing is not None:
                    existing.scores += r.scores
                else:
                    del r.fpos
                    del r.epos
                    del r.span
                    gram[r] = r

            if len(gram) >= 100000:
                log.write("dumping...\n")
                for r in gram:
                    print "%s\t%s" % (r, r.scores)
                gram.clear()
                log.write("memory: %s\n" % monitor.memory())

    if combiner:
        for r in gram:
            print "%s\t%s" % (r, r.scores)
