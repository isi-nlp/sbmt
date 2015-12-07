#!/usr/bin/env python

# extractor.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

""" would this be a faster criterion?
keep track of number of alignments in each f/e span
for each f span, find max/min aligned e words
if number of links in f span is equal to number of links in e span, then it's a phrase
"""

"""
Input:
 - a GIZA++-style alignment
 - a Pharaoh-style alignment

Output:
 - unscored grammar file
"""

"""
Future ideas:
  - change notion of "tight" so that outer phrases are minimal and inner phrases are maximal?

"""

import sys, os, os.path
import monitor
import time, math
import random
import alignment, rule, forest
import sym
import log
import cPickle

log.level = 1

PHRASE = sym.fromstring('[PHRASE]')
START = sym.fromstring('[START]')
nonterminals = [PHRASE]

class XRule(rule.Rule):
    def __init__(self, lhs, f, e, owner=None, scores=None):
        rule.Rule.__init__(self, lhs, f, e, owner=owner, scores=scores)
        self.fpos = self.epos = self.span = None

class Feature(object):
    def __init__(self):
        object.__init__(self)

    def process_alignment(self, a):
        pass

    def score_rule(self, a, r):
        return [1.0]

prefix_labels = set()
force_french_prefix = False
force_english_prefix = True

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

def loosen_french_phrases(self, phrases, maxlen):
    for (i1_max,j1,i2_min,j2) in phrases:
        i1_min = i1_max
        while i1_min > 0 and self.faligned[i1_min-1] == 0:
            i1_min -= 1
        i2_max = i2_min
        while i2_max < len(self.fwords) and self.faligned[i2_max] == 0:
            i2_max += 1

        for i1 in xrange(i1_min, i1_max+1):
            for i2 in xrange(max(i1+1,i2_min), min(i2_max,i1+maxlen)+1):
                yield (i1,j1,i2,j2)

def remove_overlaps(self, phrases):
    # Give priority to leftmost phrases. This yields a left-branching structure
    phrases = [(i1,i2,j1,j2) for (i1,j1,i2,j2) in phrases]
    phrases.sort()
    newphrases = []
    for (i1,i2,j1,j2) in phrases:
        flag = True
        for (i3,j3,i4,j4) in newphrases:
            if i1<i3<i2<i4 or i3<i1<i4<i2: # or j1<j3<j2<j4 or j3<j1<j4<j2:
                flag = False
                break
        if flag:
            newphrases.append((i1,j1,i2,j2))
    return newphrases

def label_phrase(a, (fi,fj,ei,ej)):
    if opts.etree_labels and a.espans is not None:
        return a.espans.get((ei,ej), [PHRASE])

    if opts.etree_labels:
        sys.stderr.write("this shouldn't happen either (line %d)\n" % a.lineno)
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
            for subphrase in i2index[i2]:
                (sub_x, sub_i1, sub_j1, sub_i2, sub_j2) = subphrase
                if sub_i2-sub_i1 >= minhole: # or sub_x is not PHRASE:
                    # not very efficient because no structure sharing
                    for nvars in xrange(maxvars):
                        for item in chart[i1][sub_i1][nvars]:
                            if (len(item) < maxlen
                                and not (opts.forbid_adjacent and len(item)>0 and type(item[-1]) is not int)
                                # force prefix categories to be at left edge of French side
                                and not (force_french_prefix and len(item) > 0 and sub_x in prefix_labels)
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
                    scores = [1.0]
                    for feature in features:
                        scores.extend(feature.score_rule(a, r))
                    r.scores = scores
                    result.append(r)

        # Normalize
        for r in result:
            r.scores = [x/len(result) for x in r.scores]
        wholeresult.extend(result)


    return wholeresult

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
            v = sym.setindex(sub_x, index)
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
                # force slash categories to be at left edge of English side
                if force_english_prefix and len(new_ewords) != 0 and sym.clearindex(v) in prefix_labels:
                    return None
                new_ewords.append(v)
                epos.append((ei,ej))
            else:
                new_ewords.append(ewords[i])
                epos.append(i+j1)


    r = XRule(x,rule.Phrase(tuple(fwords)), rule.Phrase(tuple(new_ewords)))
    r.fpos = fpos
    r.epos = epos
    r.span = (i1,i2,j1,j2)

    if opts.keep_word_alignments:
        r.word_alignments = []
        for fi in xrange(len(fpos)):
            if type(fpos[fi]) is int:
                for ei in xrange(len(epos)):
                    if type(epos[ei]) is int:
                        if a.aligned[fpos[fi]][epos[ei]]:
                            r.word_alignments.append((fi,ei))
    return r

def parse(n, xrules, rules):
    """
    n = length of sentence
    xrules = rules with position info, to be assembled into forest
    rules = grammar of rules from all sentences
    N.B. This does not work properly without tight_phrases"""

    chart = [[dict((v,None) for v in nonterminals) for j in xrange(n+1)] for i in xrange(n+1)]

    for l in xrange(1, n+1):
        for i in xrange(n-l+1):
            k = i+l

            for x in nonterminals:
                if x != START:
                    item = forest.Item(x,i,k)
                    for r in xrules.get((x,i,k), ()):
                        ants = []
                        for fi in xrange(len(r.f)):
                            if type(r.fpos[fi]) is tuple:
                                (subi, subj) = r.fpos[fi]
                                ants.append(chart[subi][subj][sym.clearindex(r.f[fi])])
                        if None not in ants:
                            item.derive(ants, rules[r], r.scores[0]) # the reason for the lookup in rules is to allow duplicate rules to be freed
                    if len(item.deds) == 0:
                        item = None
                    if item is not None:
                        chart[i][k][x] = item

                else: # x == START
                    item = forest.Item(x,i,k)

                    # S -> X
                    if i == 0:
                        for y in nonterminals:
                            if y != START and chart[i][k][y] is not None:
                                item.derive([chart[i][k][y]], gluestop[y])

                    # S -> S X
                    for j in xrange(i,k+1):
                        for y in nonterminals:
                            if chart[i][j][START] is not None and chart[j][k][y] is not None:
                                item.derive([chart[i][j][START], chart[j][k][y]], glue[y])

                    if len(item.deds) > 0:
                        chart[i][k][x] = item
                        for ded in item.deds:
                            ded.rule.scores = [ded.rule.scores[0] + 1./len(item.deds)]

    covered = [False] * n
    spans = []
    # find biggest nonoverlapping spans
    for l in xrange(n,0,-1):
        for i in xrange(n-l+1):
            k = i+l

            flag = False
            for v in reversed(nonterminals):
                if chart[i][k][v] is not None:
                    flag = True
            if flag:
                for j in xrange(i,k):
                    # don't let any of the spans overlap
                    if covered[j]:
                        flag = False
                if flag:
                    for j in xrange(i,k):
                        covered[j] = True
                    spans.append((i,k))

    # old buggy version
    #spans = [(0,n)]
    #sys.stderr.write("%s\n" % spans)

    # put in topological order
    itemlists = []
    for (start, stop) in spans:
        items = []
        for l in xrange(1, stop-start+1):
            for i in xrange(start, stop-l+1):
                k = i+l
                for v in nonterminals:
                    if chart[i][k][v] is not None:
                        items.append(chart[i][k][v])
        if len(items) > 0:
            itemlists.append(items)

    return itemlists

def dump_rules(rules, output, name=None):
    global dumped
    dumped += len(rules)
    outfile = file(os.path.join(output, "extract.%s" % name), "w")
    for r in rules.iterkeys():
        outfile.write("%s ||| %s\n" % (r.e.strhandle(), r.to_line()))
    outfile.close()

"""
        #del rules[r] # save memory
    lines.sort()
    if type(output) is str:
        outfile = file(os.path.join(output, "extract.%s" % name), "w")
        for line in lines:
            outfile.write(line)
        outfile.close()
    elif type(output) is file:
        for line in lines:
            output.write(line)"""

##### Features pertaining to geometry of alignments

class UnalignedFeature(Feature):
    def score_rule(self, a, r):
        funaligned = eunaligned = 0

        for i in xrange(len(r.f)):
            if not sym.isvar(r.f[i]):
                if not a.faligned[r.fpos[i]]:
                    funaligned += 1

        for i in xrange(len(r.e)):
            if not sym.isvar(r.e[i]):
                if not a.ealigned[r.epos[i]]:
                    eunaligned += 1

        return [funaligned, eunaligned]

class ChildSpanFeature(Feature):
    def ave_score_rule(self, a, r):
        spans = []
        for f in r.fpos:
            if type(f) is tuple:
                (i,j) = f
                spans.append(j-i)
                spans.append((j-i)*(j-i))
        while len(spans) < 4:
            spans.append(0)

        return spans

    def score_rule(self, a, r):
        k = 10
        spans = [0]*2*k
        vi = 0
        for f in r.fpos:
            if type(f) is tuple:
                (fi,fj) = f
                if 0 < fj-fi <= k:
                    spans[vi*k+fj-fi-1] = 1
                vi += 1
        return spans

class CrossingFeature(Feature):
    def process_alignment(self, a):
        self.cumul = compute_cumulative(a)

    def score_rule(self, a, r):
        scores = []
        scores.extend(compute_crossings(a, r, self.cumul))
        #scores.append(compute_barrier_crossings(a, r, self.cumul))
        return scores

def compute_cumulative(a):
    cumul = [[None for ei in xrange(len(a.ewords)+1)] for fi in xrange(len(a.fwords)+1)]
    for fi in xrange(len(a.fwords)+1):
        cumul[fi][0] = 0
    for ei in xrange(len(a.ewords)+1):
        cumul[0][ei] = 0
    for fi in xrange(1,len(a.fwords)+1):
        for ei in xrange(1,len(a.ewords)+1):
            cumul[fi][ei] = cumul[fi][ei-1] + cumul[fi-1][ei] - cumul[fi-1][ei-1]
            if a.aligned[fi-1][ei-1]:
                cumul[fi][ei] += 1
    return cumul

def region_aligned(cumul, fi, fj, ei, ej):
    return cumul[fj][ej] - cumul[fi][ej] - cumul[fj][ei] + cumul[fi][ei]

def compute_crossings(a, r, cumul):
    count = [0]*len(r.f)
    vars = [0,0]
    (pfi, pfj, pei, pej) = r.span
    for rfi in xrange(len(r.f)):
        for rei in xrange(len(r.e)):
            if type(r.fpos[rfi]) is int and type(r.epos[rei]) is int and a.aligned[r.fpos[rfi]][r.epos[rei]]:
                # both terminals
                fi = r.fpos[rfi]
                ei = r.epos[rei]
                if region_aligned(cumul, pfi, fi, ei+1, pej) + region_aligned(cumul, fi+1, pfj, pei, ei) > 0:
                    count[rfi] = 1
                    break
            elif type(r.fpos[rfi]) is tuple and type(r.epos[rei]) is tuple and sym.getindex(r.f[rfi]) == sym.getindex(r.e[rei]):
                # coindexed nonterminals
                (fi,fj) = r.fpos[rfi]
                (ei,ej) = r.epos[rei]
                if region_aligned(cumul, pfi, fi, ej, pej) + region_aligned(cumul, fj, pfj, pei, ei) > 0:
                    count[rfi] = 1
                    vars[sym.getindex(r.f[rfi])-1] = 1
                break
    return vars


barriers = [sym.fromstring(x) for x in '. , ( ) " : ; \357\274\233     \343\200\202     \357\274\210     \357\274\211     \357\274\214     \343\200\212     \343\200\213     \342\200\234     \342\200\235     \343\200\214     \343\200\215     \342\210\266 ']
def compute_barrier_crossings(a, r, cumul):
    count = 0
    (pfi, pfj, pei, pej) = r.span
    for rfi in xrange(len(r.f)):
        if r.f[rfi] in barriers:
            for rei in xrange(len(r.e)):
                if type(r.epos[rei]) is int and a.aligned[r.fpos[rfi]][r.epos[rei]]:
                    fi = r.fpos[rfi]
                    ei = r.epos[rei]
                    if region_aligned(cumul, pfi, fi, ei+1, pej) + region_aligned(cumul, fi+1, pfj, pei, ei) > 0:
                        count += 1
                        break
    return count

##### Syntax-related stuff

def add_multiconstituents(a, maxabslen, ephrase_index, consts):
    elen = len(a.ewords)

    chart = [[None for ej in xrange(elen+1)] for ei in xrange(elen+1)]
    for ((ei,ej),labels) in a.espans.iteritems():
        chart[ei][ej] = [labels[0]] # take the highest label

    for el in xrange(2,maxabslen+1):
        for ei in xrange(elen-el+1):
            ej = ei+el
            if chart[ei][ej] is not None: # must be a singleton
                continue
            bestsplit = None
            bestlen = None
            for ek in xrange(ei+1,ej):
                if chart[ei][ek] is not None and chart[ek][ej] is not None and (bestlen is None or len(chart[ei][ek])+len(chart[ek][ej]) < bestlen):
                    bestsplit = ek
                    bestlen = len(chart[ei][ek])+len(chart[ek][ej])
            if bestlen is not None and bestlen <= consts:
                chart[ei][ej] = chart[ei][bestsplit]+chart[bestsplit][ej]
    for (ei,ej) in ephrase_index:
        if not a.espans.has_key((ei,ej)) and chart[ei][ej] is not None:
            a.espans[ei,ej] = [sym.fromtag("_".join(sym.totag(x) for x in chart[ei][ej]))]

def add_constituent_prefixes(a, ephrase_index):
    """if a phrase is a prefix of a constituent, give it a fake label"""
    if log.level >= 3:
        log.write(str([(i,j,sym.tostring(x)) for ((i,j),l) in a.espans.iteritems() for x in l ]))
        log.write("\n")

    ei_index = {}
    for ((ei,ej),labels) in a.espans.iteritems():
        ei_index.setdefault(ei, []).extend([(ej,x) for x in reversed(labels)])
    for ei in ei_index.iterkeys():
        ei_index[ei].sort() # stable

    for (ei,ej) in ephrase_index:
        if True or not (a.espans.has_key((ei,ej)) and len(a.espans[ei,ej]) > 0):
            for (ej1,x) in ei_index.get(ei,[]):
                if ej1 > ej:
                    x1 = sym.fromtag(sym.totag(x)+"*")
                    a.espans.setdefault((ei,ej),[]).append(x1)
                    prefix_labels.add(x1)
                    break

    if log.level >= 3:
        log.write(str([(i,j,sym.tostring(x)) for ((i,j),l) in a.espans.iteritems() for x in l ]))
        log.write("\n---\n")

def remove_req(node):
    parts = node.label.split("-")
    if parts[-1] in ["HEAD", "C", "REQ"]:
        parts[-1:] = []
        node.required = True
    else:
        node.required = False
    node.label = "-".join(parts)
    for child in node.children:
        remove_req(child)

def add_sister_prefixes_helper(a, ephrases, enode, i):
    """if a phrase comprises one or more (but not all) leftmost children of a constituent, then add it and give it a fake label"""

    j = i+enode.length
    if log.level >= 3:
        log.write("(i,j) = %s\n" % ((i,j),))
    x = enode.label
    j1 = i
    for ci in xrange(len(enode.children)):
        child = enode.children[ci]
        j1 += child.length
        if log.level >= 3:
            log.write("(i,j1) = %s\n" % ((i,j1),))
        if j1 < j and (i,j1) in ephrases:

            # constprefix3:
            #x1 = sym.fromtag("%s*" % x)

            # subcat-lr2:
            #subcat = [sister.label for sister in enode.children[ci+1:] if sister.required]
            #x1 = sym.fromtag("/".join(["%s*"%x]+subcat))

            # markov1:
            x1 = sym.fromtag("%s/%s" % (x, enode.children[ci+1].label))

            # markov2:
            #x1 = sym.fromtag("%s(%s)" % (x, enode.children[ci].label))

            a.espans.setdefault((i,j1),[]).append(x1)
            prefix_labels.add(x1)

    for child in enode.children:
        add_sister_prefixes_helper(a, ephrases, child, i)
        i += child.length

def add_sister_prefixes(a, ephrases, etree):
    if log.level >= 3:
        log.write("phrases before filtering:\n")
        for (i,j) in ephrases:
            log.write("%s" % ((i,j),))
        log.write("constituents before adding:\n")
        for ((i,j),l) in a.espans.iteritems():
            log.write("%s %s\n" % ((i,j),[sym.tostring(x) for x in l]))

    add_sister_prefixes_helper(a, ephrases, etree, 0)

    if log.level >= 3:
        log.write("constituents after adding:\n")
        for ((i,j),l) in a.espans.iteritems():
            log.write("%s %s\n" % ((i,j),[sym.tostring(x) for x in l]))
        log.write("\n---\n")

def add_bounded_prefixes_helper(a, phrases, node, i, stack):
    j = i+node.length
    if node.label in ['NP']:
        stack = stack+[(node.label,i,j)]
    else:
        stack = [(node.label,i,j)]
    i1 = i
    for child in node.children:
        if i1 > i:
            for (x,i0,j0) in stack:
                if (i0,i1) in phrases:
                    x1 = sym.fromtag("%s*" % x)
                    a.espans.setdefault((i0,i1),[]).append(x1)
                    prefix_labels.add(x1)
        add_bounded_prefixes_helper(a, phrases, child, i1, stack)
        i1 += child.length

def add_bounded_prefixes(a, ephrases, etree):
    if log.level >= 3:
        log.write(str([(i,j,sym.tostring(x)) for ((i,j),l) in a.espans.iteritems() for x in l ]))
        log.write("\n")

    add_bounded_prefixes_helper(a, ephrases, etree, 0, [])

    if log.level >= 3:
        log.write(str([(i,j,sym.tostring(x)) for ((i,j),l) in a.espans.iteritems() for x in l ]))
        log.write("\n---\n")


if __name__ == "__main__":
    import gc
    gc.set_threshold(100000,10,10) # this makes a huge speed difference
    #gc.set_debug(gc.DEBUG_STATS)

    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option('-d', '--dump-size', dest='dump_size', type="int", default=1e6, help='how many rules to dump in each file')
    optparser.add_option('-L', '--maxabslen', dest='maxabslen', type="int", default=12, help='maximum initial base phrase size')
    optparser.add_option('-l', '--maxlen', dest='maxlen', type="int", default=5, help='maximum final phrase size')
    optparser.add_option('-s', '--minsublen', dest='minhole', type="int", default=1, help='minimum subphrase size')
    optparser.add_option('-v', '--maxvars', dest='maxvars', type="int", default=2, help='maximum number of nonterminals')
    optparser.add_option('-t', '--tight', dest='loosen', action='store_false', default=True, help='use tight phrases only')
    optparser.add_option('--english-loose-limit', dest='english_loose_limit', type="int")
    optparser.add_option('--french-loose-limit', dest='french_loose_limit', type="int")
    optparser.add_option('-w', '--weight', nargs=2, dest='weightfiles', help="lexical weight tables")
    optparser.add_option('-r', '--ratio', dest='ratiofile', help="likelihood ratio table")
    optparser.add_option('-W', '--words', nargs=2, dest='words', help="parallel text files (words)")
    optparser.add_option('-T', '--tags', nargs=2, dest='tags', help="parallel text files (part of speech tags)")
    optparser.add_option('-B', '--trees', nargs=2, dest='trees', help="parallel text files (trees)")
    optparser.add_option('-R', '--relabel', dest="etree_labels", action="store_true", default=False, help="relabel using English trees")
    optparser.add_option('-P', '--pharaoh', dest='pharaoh', action='store_true', default=False, help="input is Pharaoh-style alignment (requires -W)")
    optparser.add_option('-o', '--output', dest='output_dir', help="directory for output files")
    optparser.add_option('-c', '--count-crossings', dest='crossings', action="store_true", help="count number of symbols that cross a nonterminal")
    optparser.add_option('-a', '--allow-adjacent', dest="forbid_adjacent", action="store_false", default=True, help="allow adjacent nonterminals")
    optparser.add_option('-n', '--allow-nonlexical', dest="require_aligned_terminal", action="store_false", default=True, help="allow nonlexical rules")
    optparser.add_option('-p', '--parallel', dest="parallel", nargs=2, type="int", help="extract i'th sentence out of every group of j")
    optparser.add_option('-O', '--remove-overlaps', dest="remove_overlaps", help="remove overlapping phrases", action="store_true", default=False)
    optparser.add_option('-A', '--keep-word-alignments', dest="keep_word_alignments", help="keep word alignments", action="store_true", default=False)
    optparser.add_option('-F', '--output-forests', dest="output_forests", type="str", help="output forests")
    optparser.add_option('-D', '--discard-long-sentences', dest="discard_long_sentences", action="store_true", default=False, help="discard sentences longer than -L")

    (opts,args) = optparser.parse_args()

    if len(args) >= 1 and args[0] != "-":
        input_file = file(args[0], "r")
    else:
        input_file = sys.stdin

    features = []

    if opts.output_dir is not None:
        try:
            os.mkdir(opts.output_dir)
        except OSError:
            sys.stderr.write("warning: directory already exists\n")
        n_dump = 1
    else:
        output_file = sys.stdout

    if opts.words is not None:
        ffilename, efilename = opts.words
        ffile = file(ffilename)
        efile = file(efilename)

    if opts.tags is not None:
        ftfilename, etfilename = opts.tags
        ftfile = file(ftfilename)
        etfile = file(etfilename)

    if opts.trees is not None:
        import tree
        fbfilename, ebfilename = opts.trees
        if fbfilename == "none":
            fbfile = None
        else:
            fbfile = file(fbfilename)
        if ebfilename == "none":
            ebfile = None
        else:
            ebfile = file(ebfilename)

    if opts.weightfiles is not None:
        (fweightfile, eweightfile) = opts.weightfiles
    else:
        fweightfile = eweightfile = None

    import lexweights
    features.append(lexweights.LexicalWeightFeature(fweightfile, eweightfile, opts.ratiofile))

    if opts.crossings:
        #features.append(CrossingFeature())
        features.append(ChildSpanFeature())

    maxlen = opts.maxlen
    maxabslen = opts.maxabslen

    dumped = 0

    prev_time = start_time = time.time()
    slice = 1000

    if log.level >= 1:
        sys.stderr.write("(2) Extracting rules\n")
    count = 1
    realcount = 0
    slice = 1000
    if opts.pharaoh:
        alignments = alignment.Alignment.reader_pharaoh(ffile, efile, input_file)
    else:
        alignments = alignment.Alignment.reader(input_file)
        # bug: ignores -W option

    gram = {}

    if opts.output_forests:
        pickler = cPickle.Pickler(file(opts.output_forests, "w") , cPickle.HIGHEST_PROTOCOL)

    for a in alignments:
        a.lineno = count

        if log.level >= 2:
            a.write(log.file)
            a.write_visual(log.file)
            log.file.flush()

        if opts.tags:
            a.ftags = ftfile.readline().split()
            a.etags = etfile.readline().split()
            if len(a.ftags) != len(a.fwords):
                sys.stderr.write("warning: length mismatch between French words and tags (%d != %d)\n" % (len(a.ftags), len(a.fwords)))
            if len(a.etags) != len(a.ewords):
                sys.stderr.write("warning: length mismatch between English words and tags (%d != %d)\n" % (len(a.etags), len(a.ewords)))

        a.espans = None
        if opts.trees:
            if ebfile is not None:
                etree = tree.str_to_tree(ebfile.readline())
                if etree is None:
                    sys.stderr.write("warning, line %d: null tree" % a.lineno)
                    a.espans = {}
                elif etree.length != len(a.ewords):
                    sys.stderr.write("warning, line %d: length mismatch between English words and trees (%d != %d)\n" % (a.lineno, len(a.ewords), etree.length))
                    sys.stderr.write("  start of English sentence: %s\n" % " ".join([sym.tostring(x) for x in a.ewords[:5]]))
                    a.espans = {}
                else:
                    remove_req(etree)
                    a.espans = etree.spans()
                    for (span, labels) in a.espans.iteritems():
                        a.espans[span] = [sym.fromtag(x) for x in labels]

        # done reading all input lines
        if opts.discard_long_sentences and len(a.fwords) > opts.maxabslen:
            continue

        realcount += 1
        if opts.parallel is not None:
            if realcount % opts.parallel[1] != opts.parallel[0] % opts.parallel[1]:
                continue

        for feature in features:
            feature.process_alignment(a)

        phrases = extract_phrases(a, maxabslen)
        if opts.loosen:
            phrases = loosen_phrases(a, phrases, maxabslen, opts.french_loose_limit, opts.english_loose_limit)

        if opts.remove_overlaps:
            phrases = remove_overlaps(a, phrases)

        phrases = list(phrases)
        ephrase_index = set()
        for (fi,ei,fj,ej) in phrases:
            ephrase_index.add((ei,ej))

        if a.espans is not None and len(a.espans) > 0: # empty espans suggests an error
            #add_multiconstituents(a, maxabslen, ephrase_index, 2)
            #add_constituent_prefixes(a, ephrase_index)
            add_sister_prefixes(a, ephrase_index, etree)
            #add_bounded_prefixes(a, ephrase_index, etree)
            #pass

        # filter using English trees
        if a.espans is not None:
            #phrases = list(phrases)
            phrases = [(fi,ei,fj,ej) for (fi,ei,fj,ej) in phrases if a.espans.has_key((ei,ej))]
            #phrases = random.sample(phrases, len(dummy_phrases))

        if log.level >= 3:
            sys.stderr.write("Initial phrases:\n")
            phrases = list(phrases)
            for (i1,j1,i2,j2) in phrases:
                sys.stderr.write("(%d,%d) %s (%d,%d) %s\n" % (i1,i2," ".join([sym.tostring(w) for w in a.fwords[i1:i2]]),j1,j2," ".join([sym.tostring(w) for w in a.ewords[j1:j2]])))

        labeled = []
        for (fi,ei,fj,ej) in phrases:
            labeled.extend((x,fi,ei,fj,ej) for x in label_phrase(a, (fi,fj,ei,ej)))
        phrases = labeled

        rules = subtract_phrases(a, phrases, maxlen, opts.minhole, opts.maxvars)

        if log.level >= 3:
            sys.stderr.write("Rules:\n")
            rules = list(rules)
            for r in rules:
                sys.stderr.write("%d ||| %s\n" % (realcount, r))

        if False:
            rules = list(rules)
            for r in rules:
                sys.stderr.write("%d ||| %s ||| %f %f\n" % (realcount-1, r, r.scores[1]/r.scores[0], r.scores[2]/r.scores[0]))

        rs = {}
        for r in rules:
            if opts.output_forests:
                (fi,fj,ei,ej) = r.span
                rs.setdefault((r.lhs,fi,fj), []).append(r)

            existing = gram.get(r, None)
            if existing is not None:
                existing += r
            else:
                # throw away the extended info to save memory.
                r = rule.Rule(r.lhs, r.f, r.e, owner=r.owner, scores=r.scores, word_alignments=r.word_alignments)
                gram[r] = r

        if opts.output_forests:
            chart = parse(len(a.fwords), rs, gram)
            pickler.dump(chart)
            pickler.clear_memo()

        if opts.output_dir is not None and len(gram) >= opts.dump_size:
            if opts.parallel:
                name = "%04d.%04d" % (opts.parallel[0], n_dump)
            else:
                name = "%04d" % n_dump
            dump_rules(gram, opts.output_dir, name)
            dumped += len(gram)
            gram = {}
            n_dump += 1

        if log.level >= 1 and count%slice == 0:
            sys.stderr.write("time: %f, sentences in: %d (%.1f/sec), " % (time.time()-start_time, count, slice/(time.time()-prev_time)))
            sys.stderr.write("rules out: %d+%d\n" % (dumped, len(gram)))
            sys.stderr.write("memory: %s\n" % monitor.memory())
            prev_time = time.time()

        count += 1

    if opts.output_dir is not None:
        if opts.parallel:
            name = "%04d.%04d" % (opts.parallel[0], n_dump)
        else:
            name = "%04d" % n_dump
        dump_rules(gram, opts.output_dir, name)
    else:
        dump_rules(gram, output_file)

    """if opts.output_forests:
        pickler.dump(sym.alphabet)"""
