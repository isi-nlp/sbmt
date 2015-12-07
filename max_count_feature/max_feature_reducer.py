#!/usr/bin/env python

import sys, itertools
from decimal import *

# requires mapreduce be specified to use two keys for sorting, 1 for partitioning

def keyval(input):
    for line in input:
        args = line.strip().split('\t')
        k = args[0]
        w = args[-1]
        v = '\t'.join(args[1:len(args)-1])
        yield k.strip(),v.strip(),Decimal(w)

def reducer(pairs):
    for rule, rec in itertools.groupby(pairs, lambda x : x[0]):
        occ = Decimal(0)
        val = ''
        for fval, sub in itertools.groupby(rec, lambda x : x[1]):
            fvoc = Decimal(0)
            for ignore1,ignore2,w in sub:
                fvoc += w
            if fvoc > occ:
                occ = fvoc
                val = fval
        yield rule, val

if __name__ == '__main__':
    for rule, val in reducer(keyval(sys.stdin)):
        print "%s\t%s" % (rule,val)



        
