import sys

for line in sys.stdin:
    fields = line.rstrip().split("\t")
    words = fields[0].split()
    print '\t'.join([" ".join(words[1:]), words[0]] + fields[1:])
