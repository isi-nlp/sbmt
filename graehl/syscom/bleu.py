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
import sys, math, re, xml.sax.saxutils

verbose = False
preserve_case = False
eff_ref_len = "shortest"
nist_tokenize = True
clip_len = False
show_length_ratio = False

normalize1 = [
    ('<skipped>', ''),         # strip "skipped" tags
    (r'-\n', ''),              # strip end-of-line hyphenation and join lines
    (r'\n', ' '),              # join lines
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
    s = xml.sax.saxutils.unescape(s, {'&quot;':'"'}) # &amp; &lt; &gt; ?
    # language-dependent part (assuming Western languages):
    s = " %s " % s
    if not preserve_case:
        s = s.lower()         # this might not be identical to the original
    for (pattern, replace) in normalize2:
        s = re.sub(pattern, replace, s)
    return s.split()

def precook(s, n=4):
    """Takes a string as input and returns an object that can be given to
    either cook_refs or cook_test. This is optional: cook_refs and cook_test
    can take string arguments as well."""
    if type(s) is str:
        if nist_tokenize:
            words = normalize(s)
        else:
            words = s.split()
        counts = {}
        for k in xrange(1,n+1):
            for i in xrange(len(words)-k+1):
                ngram = tuple(words[i:i+k])
                counts[ngram] = counts.get(ngram, 0)+1
        return (len(words), counts)
    else:
        return s

def cook_refs(refs, n=4):
    '''Takes a list of reference sentences for a single segment
    and returns an object that encapsulates everything that BLEU
    needs to know about them.'''

    reflen = []
    maxcounts = {}
    for ref in refs:
        rl, counts = precook(ref, n)
        reflen.append(rl)
        for (ngram,count) in counts.iteritems():
            maxcounts[ngram] = max(maxcounts.get(ngram,0), count)

    # Calculate effective reference sentence length.
    if eff_ref_len == "shortest":
        reflen = min(reflen)
    elif eff_ref_len == "average":
        reflen = float(sum(reflen))/len(reflen)

    return (reflen, maxcounts)

def cook_test(test, (reflen, refmaxcounts), n=4):
    '''Takes a test sentence and returns an object that
    encapsulates everything that BLEU needs to know about it.'''

    testlen, counts = precook(test, n)

    result = {}

    # Calculate effective reference sentence length.
    
    if eff_ref_len == "closest":
        result["reflen"] = min((abs(l-testlen), l) for l in reflen)[1]
    else:
        result["reflen"] = reflen

    if clip_len:
        result["testlen"] = min(testlen, result["reflen"])
    else:
        result["testlen"] = testlen

    result["guess"] = [max(0,testlen-k+1) for k in xrange(1,n+1)]

    result['correct'] = [0]*n
    for (ngram, count) in counts.iteritems():
        result["correct"][len(ngram)-1] += min(refmaxcounts.get(ngram,0), count)

    return result

def score_cooked(allcomps, n=4, addprec=0):
    totalcomps = {'testlen':0, 'reflen':0, 'guess':[0]*n, 'correct':[0]*n}
    for comps in allcomps:
        for key in ['testlen','reflen']:
            totalcomps[key] += comps[key]
        for key in ['guess','correct']:
            for k in xrange(n):
                totalcomps[key][k] += comps[key][k]
    bleu = 1.
    for k in xrange(n):
        bleu *= float(totalcomps['correct'][k]+addprec)/(totalcomps['guess'][k]+addprec)
    bleu = bleu ** (1./n)
    if 0 < totalcomps['testlen'] < totalcomps['reflen']:
        bleu *= math.exp(1-float(totalcomps['reflen'])/totalcomps['testlen'])

    if verbose:
        sys.stderr.write("hyp words: %s\n" % totalcomps['testlen'])
        sys.stderr.write("effective reference length: %s\n" % totalcomps['reflen'])
        

    if verbose:
        for k in xrange(n):
            prec = float(totalcomps['correct'][k]+addprec)/(totalcomps['guess'][k]+addprec)
            sys.stderr.write("%d-gram precision:  %s\n" % (k+1,prec))
    if verbose or show_length_ratio:
        sys.stderr.write("length ratio:      %s\n" % (float(totalcomps['testlen'])/totalcomps['reflen']))
        
    return bleu

if __name__ == "__main__":
    import getopt, itertools

    (opts,args) = getopt.getopt(sys.argv[1:], "r:ctplv", [])
    for (opt,parm) in opts:
        if opt == "-r":
            eff_ref_len = parm
        elif opt == "-c":
            preserve_case = True
        elif opt == "-t":
            nist_tokenize = False
        elif opt == "-p":
            clip_len = True
        elif opt == "-l":
            show_length_ratio = True
        elif opt == "-v":
            verbose = True

    if args[0] == '-':
        infile = sys.stdin
    else:
        infile = file(args[0])

    refs = []
    reffilenames = args[1:]
    for lines in itertools.izip(*[file(filename) for filename in reffilenames]):
        refs.append(cook_refs(lines))
        
    tests = []
    #for (i,line) in enumerate(infile):
    for ref in refs:
        line = infile.readline() # blank lines if infile is short
        tests.append(cook_test(line, ref))

    print score_cooked(tests)
    
            
    
