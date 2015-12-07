#!/home/nlg-01/chiangd/pkg/python/bin/python

import sys
import itertools

def input():
    for line in sys.stdin:
        fields = line.rstrip().split("\t")
        key = "\t".join(fields[:-1])
        count = int(fields[-1])
        yield key, count

for key, lines in itertools.groupby(input(), lambda x: x[0]):
    count = sum(count for _, count in lines)
    print "\t".join([key] + [str(count)])
        
