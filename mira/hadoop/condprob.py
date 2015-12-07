#!/home/nlg-01/chiangd/pkg/python/bin/python

"""
Input:
  key \t [LHS] ||| frhs ||| erhs ||| attrs \t scores

Output:
  for each key, sum the counts of all rules, then output
  all rules with a new feature = -log(count/sum)
"""

import sys
import math
import itertools
import svector

if __name__ == "__main__":
    import getopt

    feat_ncount = feat_dcount = "count"
    feat_prob = "prob"

    opts, args = getopt.getopt(sys.argv[1:], "n:d:p:")
    for o, a in opts:
        if o == "-n":
            feat_ncount = a
        elif o == "-d":
            feat_dcount = a
        elif o == "-p":
            feat_prob = a

    if len(args) > 0:
        feat_prob = args[0] # backwards compatibility

    def input():
        for line in sys.stdin:
            try:
                key, rule, scores = line.split("\t")
            except Exception:
                sys.stderr.write("bad line: %s\n" % line.rstrip())
                raise
            scores = svector.Vector(scores)
            if feat_prob in scores:
                raise Exception("feature %s already present" % feat_prob)
            yield key, rule, scores
    
    for _, rules in itertools.groupby(input(), lambda x: x[0]):
        rules = list(rules) # this could use a lot of memory!
        sumcount = sum(scores[feat_dcount] for _, _, scores in rules)
        for _, rule, scores in rules:
            scores[feat_prob] = -math.log10(scores[feat_ncount]/sumcount)
            print "%s\t%s" % (rule.strip(), scores)
            
