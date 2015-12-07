#!/usr/bin/env python
import sys, itertools

for lines in itertools.izip(*[file(fn) for fn in sys.argv[1:]]):
    # make sure space is the only whitespace before pasting
    print "\t".join(" ".join(line.split()) for line in lines)
