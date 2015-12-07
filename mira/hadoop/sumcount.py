#!/home/nlg-01/chiangd/pkg/python/bin/python

"""
Input:
  key \t [LHS] ||| frhs ||| erhs ||| attrs \t scores

Output:
  [LHS] ||| frhs ||| erhs ||| attrs \t scores count=(sum of counts)
"""

import sys
import itertools
import collections
import svector
import simplerule

def input():
    for line in sys.stdin:
        key, rule, scores = line.rsplit("\t", 2)
        scores = svector.Vector(scores)
        yield key, simplerule.Rule.from_str(rule), scores

if __name__ == "__main__":
    import getopt
    
    feat_in = feat_out = "count"
    opts, args = getopt.getopt(sys.argv[1:], "i:o:")
    for o, a in opts:
        if o == "-i":
            feat_in = a
        elif o == "-o":
            feat_out = a

    for baserule, rules in itertools.groupby(input(), lambda x: x[0]):
        sumcount = 0.
        rules = list(rules)
        for _, rule, scores in rules:
            sumcount += scores[feat_in]
        for _, rule, scores in rules:
            scores[feat_out] = sumcount
            print "%s\t%s" % (rule, scores)
