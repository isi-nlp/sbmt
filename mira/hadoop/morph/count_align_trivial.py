import sys, getopt, itertools

# input: fwords ewords

class SkipSentence(Exception):
    pass

inverse = False # P(f|e)
opts, args = getopt.getopt(sys.argv[1:], "i")
for o, a in opts:
    if o == "-i":
        inverse = True

for line in sys.stdin:
    try:
        fields = line.split("\t")
        fwords, ewords = fields
        fwords = fwords.split()
        ewords = ewords.split()
        if inverse:
            fwords, ewords = ewords, fwords

        if len(fwords) != len(ewords):
            sys.stderr.write("length unmatch\n")
            raise SkipSentence

        for fword, eword in itertools.izip(fwords, ewords):
            print "%s\t%s\t%s" % (fword, eword, 1)
        print "NULL\tNULL\t1"

    except SkipSentence:
        continue

