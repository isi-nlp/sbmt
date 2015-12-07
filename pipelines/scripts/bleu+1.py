#!/usr/usc/python/2.6.5/bin/python
#!/home/nlg-01/chiangd/pkg/python/bin/python2.4
# bleu+1.py
# 4 Apr 2006
# David Chiang

import sys, itertools, math
import optparse

import bleu

# turn off NIST normalization
def normalize(words):
    return words

bleu.normalize = normalize

# usage: bleu+1.py <test> <ref>+

if __name__ == "__main__":
    optparser = optparse.OptionParser()
    optparser.add_option("-m", "--map-file", dest="mapfilename", help="map file indicating sentence number in reference set for each line of input")
    optparser.add_option("-b", "--brevity-penalty", dest="brevitypenalty", action="store_true", help="assess brevity penalty")
    (opts, args) = optparser.parse_args()

    n = 4

    cookedrefs = []
    for lines in itertools.izip(*[file(filename) for filename in args[1:]]):
        cookedrefs.append(bleu.cook_refs([line.split() for line in lines], n=n))

    if opts.mapfilename is not None:
        linemap = []
        for line in file(opts.mapfilename):
            linemap.append(int(line))
    else:
        linemap = range(len(cookedrefs))

    test1 = []
    for (line,i) in itertools.izip(file(args[0]), linemap):
        test1.append(bleu.cook_test(line.split(), cookedrefs[i], n=n))

    total = 0.
    n_sent = 0

    for comps in test1:
        if comps['testlen'] == 0:
            sys.stdout.write("0\n")
            continue
        logbleu = 0.0
        for k in xrange(n):
            logbleu += math.log(comps['correct'][k]+1)-math.log(comps['guess'][k]+1)
            #sys.stdout.write("%d/%d " % (comps['correct'][k], comps['guess'][k]))
        logbleu /= float(n)

        if opts.brevitypenalty:
            logbleu += min(0,1-(float(comps['reflen']+1))/(comps['testlen']+1))

        score = math.exp(logbleu)
        sys.stdout.write("%f\n" % score)

        total += score
        n_sent += 1

    sys.stderr.write("average: %s\n" % (total/n_sent))

