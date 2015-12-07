#!/usr/bin/env python

# bleu.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

'''Provides:

cook_refs(refs, n=4): Transform a list of reference sentences as strings into a form usable by cook_test().
cook_test(test, refs, n=4): Transform a test sentence as a string (together with the cooked reference sentences) into a form usable by score_cooked().
score_cooked(alltest, n=4): Score a list of cooked test sentences.

The reason for breaking the BLEU computation into three phases cook_refs(), cook_test(), and score_cooked() is to allow the caller to calculate BLEU scores for multiple test sets as efficiently as possible.
'''

import optparse
import sys, math, re

preserve_case = False
eff_ref_len = "shortest"
nist_tokenize = True
clip_len = False

normalize1 = [
    ('<skipped>', ''),         # strip "skipped" tags
    (r'-\n', ''),              # strip end-of-line hyphenation and join lines
    (r'\n', ' '),              # join lines
    ('&quot;', '"'),           # avoid XML modules
#    (r'(\d)\s+(?=\d)', r'\1'), # join digits
]
normalize1 = [(re.compile(pattern), replace) for (pattern, replace) in normalize1]

normalize2 = [
    (r'([\{-\~\[-\` -\&\(-\+\:-\@\/])',r' \1 '), # tokenize punctuation. apostrophe is missing
    (r'([^0-9])([\.,])',r'\1 \2 '),              # tokenize period and comma unless preceded by a digit
    (r'([\.,])([^0-9])',r' \1 \2'),              # tokenize period and comma unless followed by a digit
    (r'([0-9])(-)',r'\1 \2 ')                    # tokenize dash when preceded by a digit
]
normalize2 = [(re.compile(pattern), replace) for (pattern, replace) in normalize2]

def normalize(s):
    '''Normalize and tokenize text. This is lifted from NIST mteval-v11a.pl.'''
    if type(s) is not str:
        s = " ".join(s)
    # language-independent part:
    for (pattern, replace) in normalize1:
        s = re.sub(pattern, replace, s)
    # replaced with pattern in normalize1 to avoid pulling in XML 
    # s = xml.sax.saxutils.unescape(s, {'&quot;':'"'})
    # language-dependent part (assuming Western languages):
    s = " %s " % s
    if not preserve_case:
        s = s.lower()         # this might not be identical to the original
    for (pattern, replace) in normalize2:
        s = re.sub(pattern, replace, s)
    return s.split()

def count_ngrams(words, n=4):
    counts = {}
    for k in xrange(1,n+1):
        for i in xrange(len(words)-k+1):
            ngram = tuple(words[i:i+k])
            counts[ngram] = counts.get(ngram, 0)+1
    return counts

def cook_refs(refs, n=4):
    '''Takes a list of reference sentences for a single segment
    and returns an object that encapsulates everything that BLEU
    needs to know about them.'''

    if nist_tokenize:
        refs = [normalize(ref) for ref in refs]
    else:
        refs = [ref.split() for ref in refs]
    maxcounts = {}
    for ref in refs:
        counts = count_ngrams(ref, n)
        for (ngram,count) in counts.iteritems():
            maxcounts[ngram] = max(maxcounts.get(ngram,0), count)
    return ([len(ref) for ref in refs], maxcounts)

def cook_test(test, (reflens, refmaxcounts), n=4):
    '''Takes a test sentence and returns an object that
    encapsulates everything that BLEU needs to know about it.'''

    if nist_tokenize:
        test = normalize(test)
    else:
        test = test.split()
    result = {}

    # Calculate effective reference sentence length.
    
    if eff_ref_len == "shortest":
        result["reflen"] = min(reflens)
    elif eff_ref_len == "average":
        result["reflen"] = float(sum(reflens))/len(reflens)
    elif eff_ref_len == "closest":
        result["reflen"] = min((abs(l-len(test)), l) for l in reflens)[1]
    else:
        sys.stderr.write("unknown effective reference length method: %s\n" % eff_ref_len)
        raise ValueError

    if clip_len:
        result["testlen"] = min(len(test), result["reflen"])
    else:
        result["testlen"] = len(test)

    result["guess"] = [max(0,len(test)-k+1) for k in xrange(1,n+1)]

    result['correct'] = [0]*n
    counts = count_ngrams(test, n)
    for (ngram, count) in counts.iteritems():
        result["correct"][len(ngram)-1] += min(refmaxcounts.get(ngram,0), count)

    return result

def score_cooked(allcomps, n=4):
    totalcomps = {'testlen':0, 'reflen':0, 'guess':[0]*n, 'correct':[0]*n}
    for comps in allcomps:
        for key in ['testlen','reflen']:
            totalcomps[key] += comps[key]
        for key in ['guess','correct']:
            for k in xrange(n):
                totalcomps[key][k] += comps[key][k]
    logbleu = 0.0
    for k in xrange(n):
        if totalcomps['correct'][k] == 0:
            return 0.0
        logbleu += math.log(totalcomps['correct'][k])-math.log(totalcomps['guess'][k])
    logbleu /= float(n)
    logbleu += min(0,1-float(totalcomps['reflen'])/totalcomps['testlen'])
    return math.exp(logbleu)

if __name__ == "__main__":
    import getopt, itertools

    (opts,args) = getopt.getopt(sys.argv[1:], "rctp", [])
    for (opt,parm) in opts:
        if opt == "-r":
            raw_test = True
        elif opt == "-c":
            preserve_case = True
        elif opt == "-t":
            nist_tokenize = False
        elif opt == "-p":
            clip_len = True

    if args[0] == '-':
        infile = sys.stdin
    else:
        infile = file(args[0])

    refs = []
    reffilenames = args[1:]
    for lines in itertools.izip(*[file(filename) for filename in reffilenames]):
        refs.append(cook_refs(lines))
        
    tests = []
    for (i,line) in enumerate(infile):
        tests.append(cook_test(line, refs[i]))

    print score_cooked(tests)
    
            
    
