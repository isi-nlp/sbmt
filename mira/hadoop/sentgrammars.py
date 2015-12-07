import sys
import os

outdir = sys.argv[1]
files = {}

for line in sys.stdin:
    i, rule = line.split('\t', 1)
    if i not in files:
        if i == "global":
            files[i] = open(os.path.join(outdir, "grammar.global"), "w")
        else:
            files[i] = open(os.path.join(outdir, "grammar.%s"%i), "w")
        sys.stderr.write("creating grammar for sentence %s\n" % i)
    files[i].write(rule)
    
