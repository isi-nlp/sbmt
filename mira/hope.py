#!/usr/bin/env python

import sys
import itertools
import optparse

import forest
import rule
import oracle
import decoder
import sym
import svector
import sgml

# usage: hope.py <forest> <source> <ref>+

optparser = optparse.OptionParser()
optparser.add_option("-w", "--weights", dest="weights", help="weights")
opts, args = optparser.parse_args()

# Feature weights
weights = svector.Vector("lm1=0.1 gt_prob=0.1")
if opts.weights:
    weights = svector.Vector(opts.weights)

theoracle = oracle.Oracle(4, variant="ibm")

srcfilename = args[1]
forestfilename = args[0]
reffilenames = args[2:]

srcfile = open(srcfilename)
forestfile = open(forestfilename) if forestfilename != "-" else sys.stdin
reffiles = [open(reffilename) for reffilename in reffilenames]

def output(f):
    deriv = f.viterbi_deriv()
    hypv = deriv.vector()
    hyp = deriv.english()
    return "hyp={{{%s}}} derivation={{{%s}}} %s" % (" ".join(sym.tostring(e) for e in hyp), deriv, hypv)

for srcline, forestline, reflines in itertools.izip(srcfile, forestfile, itertools.izip(*reffiles)):
    f = forest.forest_from_text(forestline)

    # the oracle needs to know how long all the French spans are
    for item in f.bottomup():
        for ded in item.deds:
            # replace rule's French side with correct number of French words
            # we don't even bother to use the right number of variables
            ded.rule = rule.Rule(ded.rule.lhs,
                                 rule.Phrase([sym.fromstring('<foreign-word>')]*int(ded.dcost['foreign-length'])),
                                 ded.rule.e)

    f.reweight(weights)
    print "1-best %s" % output(f)

    s = sgml.Sentence(srcline.split())
    s.fwords = srcline.split()
    s.refs = [refline.split() for refline in reflines]
    theoracle.input(s, verbose=False)

    oracleweights = theoracle.make_weights(additive=True)
    # we use the in-place operations because oracleweights might be
    # a subclass of Vector
    oracleweights *= -1
    oracleweights += weights
    
    f.rescore(theoracle.models, oracleweights, add=True)
    print "hope %s" % output(f)

    oracleweights = theoracle.make_weights(additive=True)
    oracleweights += weights

    f.reweight(oracleweights)
    print "fear %s" % output(f)
