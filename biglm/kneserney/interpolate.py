#/usr/bin/env python

# stdin:
#   word2...wordn \t word1 \t prob \t bow \t prob'

# stdout:
#   word1...wordn \t prob+bow*prob'

import sys

for line in sys.stdin:
    suffix, word1, prob, bow, boprob = line.rstrip().split('\t')
    prob = float(prob)
    if prob > 0.:
        print "%s %s\t%s" % (word1, suffix, prob+float(bow)*float(boprob))
        
    
