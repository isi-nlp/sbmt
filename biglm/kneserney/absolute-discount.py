#!/usr/bin/env python

import sys, itertools, collections

d_max = 3
#mincount = [0,1,1,2,2,2,2,2,2] # this is the SRI-LM default
#mincount = [0,1,1,1,1,1,1,1,1] # no pruning
mincount = [0,1,1,1,1,2,2,2,2] # prune singleton 5-grams

def input():
    for line in sys.stdin:
        context, event, scount = line.rstrip().split('\t')
        yield context, event, int(scount)

if __name__ == "__main__":
    countcount = collections.defaultdict(int)
    for line in open(sys.argv[1]):
        c, cc = [int(s) for s in line.rstrip().split('\t')]
        if c <= d_max+1:
            countcount[c] = cc

    y = float(countcount[1])/(countcount[1]+2*countcount[2])

    # Original Kneser-Ney
    d = [0., y]

    # Modified Kneser-Ney
    d = [0.]+[i-(i+1)*y*countcount[i+1]/countcount[i] for i in xrange(1,d_max+1)]

    sys.stderr.write("Kneser-Ney: D=%s\n" % d)

    for context, records in itertools.groupby(input(), lambda record: record[0]):
        records = list(records)
        sumcount = sum(count for _, _, count in records)
        order = len(context.split())+1
        
        for _, event, count in records:
            if count < mincount[order]:
                prob = 0.
            else:
                discount = d[count] if count < len(d) else d[-1]
                prob = (count-discount)/sumcount

            if context:
                print '%s %s\t%s' % (context, event, prob)
            else:
                print '%s\t%s' % (event, prob)
                
