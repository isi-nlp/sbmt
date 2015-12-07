import sys, getopt, collections

# input: fwords ewords align

inverse = False # P(f|e)
opts, args = getopt.getopt(sys.argv[1:], "i")
for o, a in opts:
    if o == "-i":
        inverse = True

for line in sys.stdin:
    fields = line.split("\t")
    if len(fields) == 3:
        fwords, ewords, align = fields
        tags = ""
    elif len(fields) == 4:
        fwords, ewords, align, tags = fields
    fwords = fwords.split()
    ewords = ewords.split()
    tags = tags.split()
    tags.append("*")
    align = [tuple(int(i) for i in a.split("-")) for a in align.split()]
    if inverse:
        fwords, ewords = ewords, fwords
        align = [(fi,ei) for (ei,fi) in align]

    fertility = collections.defaultdict(int)
    for fi,ei in align:
        fertility[ei] += 1

    for tag in tags:
        for fi,ei in align:
            print "%s %s\t%s\t%s" % (tag, fwords[fi], ewords[ei], 1./fertility[ei])
        for ei in xrange(len(ewords)):
            if fertility[ei] == 0:
                print "%s NULL\t%s\t%s" % (tag, ewords[ei], 1.)
