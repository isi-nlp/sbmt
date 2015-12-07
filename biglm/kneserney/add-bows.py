#!/usr/bin/env python

import sys, itertools, collections, getopt

def input():
    for line in sys.stdin:
        context, event, sprob = line.rstrip().split('\t')
        yield context, event, float(sprob)

if __name__ == "__main__":
    opts, args = getopt.gnu_getopt(sys.argv[1:], 'j')
    opts = dict(opts)
    
    for context, records in itertools.groupby(input(), lambda record: record[0]):
        records = list(records)
        sumprob = sum(prob for _, _, prob in records)

        if '-j' not in opts:
            print '%s\t%s' % (context, 1.-sumprob)
        else:
            for _, event, prob in records:
                print '%s %s\t%s\t%s' % (context, event, prob, 1.-sumprob)
                    
