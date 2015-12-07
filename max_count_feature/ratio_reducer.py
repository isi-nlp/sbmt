#!/usr/bin/env python

import sys, itertools, collections

# requires mapreduce be specified to use two keys for sorting, 1 for partitioning

def keyval(input):
    for line in input:
        r,k,v = line.strip().split('\t')
        yield r.strip(),k.strip(),v.strip()

def reducer(pairs):
    for rule, rec in itertools.groupby(pairs, lambda x : x[0]):
        total = 0
        m = collections.defaultdict(int)
        for r,k,v in rec:
            m['%s-%s' % (k,v)] += 1
            total += 1
        M = {}
        for k,w in m.iteritems():
            M[k] = float(w)/float(total)
        yield rule,M 
            

if __name__ == '__main__':
    for rule, M in reducer(keyval(sys.stdin)):
        print rule + '\t' + ' '.join('%s=10^-%g' % p for p in M.iteritems())
