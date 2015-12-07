#!/home/nlg-01/chiangd/pkg/python/bin/python

"""
Input:
  key \t [LHS] ||| frhs ||| erhs ||| attrs \t scores

Output:
  the key should be the rule without attrs
  for each key,
    for each attr, pick the value with highest count
    with the *sum* of the scores of all rules
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
    for baserule, rules in itertools.groupby(input(), lambda x: x[0]):
        # form union of all attributes
        attrs = collections.defaultdict(lambda: collections.defaultdict(int))
        sumscores = svector.Vector()
        for _, rule, scores in rules:
            for attr, value in rule.attrs.iteritems():
                attrs[attr][value] += scores['count']
            sumscores += scores

        # for each attribute, select most frequent value
        rule = simplerule.Rule.from_str(baserule)
        for attr in attrs:
            _, rule.attrs[attr] = max((count, value) for (value, count) in attrs[attr].iteritems())

        print "%s\t%s" % (rule, sumscores)
