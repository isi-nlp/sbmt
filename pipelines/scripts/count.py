#!/home/nlg-01/chiangd/pkg/python/bin/python

# usage: count.py -k <k>

# input:  key \t value \t count
# output: key \t sum

import sys
import itertools
import getopt

def input():
    for line in sys.stdin:
        fields = line.rstrip().split("\t")
        yield fields
        
opts, args = getopt.gnu_getopt(sys.argv[1:], "k:u")
opts = dict(opts)

n_keys = int(opts.get('-k',1))
def get_key(record):
    return record[:n_keys]

def get_count(record):
    if '-u' in opts: # supply implicit count of 1
        return 1
    else:
        return int(record[-1])

for key, records in itertools.groupby(input(), get_key):
    sumcount = sum(get_count(record) for record in records)
    print "%s\t%s" % ("\t".join(key), sumcount)
        
