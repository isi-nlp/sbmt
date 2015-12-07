#!/home/nlg-01/chiangd/pkg/python/bin/python

"""
Input:
  [LHS] ||| frhs ||| erhs ||| attrs \t scores

Output:
  for each rule, output with the *sum* of the scores of all rules
"""

import sys
import itertools
import svector

def input():
    for line in sys.stdin:
        rule, scores = line.rsplit("\t", 1)
        scores = svector.Vector(scores)
        yield rule, scores

if __name__ == "__main__":
    for rule, counts in itertools.groupby(input(), lambda x: x[0]):
        scores = sum((scores for _, scores in counts), svector.Vector())
        print "%s\t%s" % (rule.strip(), scores)
        
        
