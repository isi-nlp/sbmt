import sys, math, getopt

opts, args = getopt.getopt(sys.argv[1:], "b")
opts = dict(opts)

for line in sys.stdin:
    fields = line.rstrip().split('\t')
    ngram = fields[0]
    try:
        prob = math.log10(float(fields[1]))
    except:
        prob = -99 # imitate SRI-LM

    if "-b" in opts:
        try:
            bow = math.log10(float(fields[2]))
        except:
            bow = 0
    else:
        bow = None

    if bow is not None:
        print "%s\t%s\t%s" % (prob, ngram, bow)
    else:
        print "%s\t%s" % (prob, ngram)
