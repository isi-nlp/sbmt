#!/usr/bin/env python

import sys
import re


if __name__ == "__main__":
    import sys
    import optparse
    import manku
    
    import gc
    gc.disable()

    optparser = optparse.OptionParser()
    optparser.add_option("-p", "--prob-quanta", dest="prob_quanta", type=int, help="number of quanta for probabilities")
    optparser.add_option("-b", "--bow-quanta", dest="bow_quanta", type=int, help="number of quanta for backoff weights")
    optparser.add_option("-P", "--prob-bits", dest="prob_bits", type=int, help="number of bits for probabilities")
    optparser.add_option("-B", "--bow-bits", dest="bow_bits", type=int, help="number of bits for backoff weights")
    optparser.add_option("-n", "--order", dest="order", type=int, help="order of language model (required)")
    (opts, args) = optparser.parse_args()

    if opts.prob_quanta is None:
        opts.prob_quanta = 2**opts.prob_bits
    if opts.bow_quanta is None:
        opts.bow_quanta = 2**opts.bow_bits

    filename = args[0]
    sys.stderr.write("scanning %s\n" % filename)

    countre = re.compile(r"ngram (\d+)=(\d+)")
    sectionre = re.compile(r"\\(\d+)-grams:")
    count = {}
    order = None

    state = 0
    for line in file(filename):
        if line.startswith("\\"):
            m = sectionre.match(line)

            if state > 0 and m is None and line.strip() != "\\end\\":
                sys.stderr.write("oh, i thought this wasn't supposed to happen: %s\n" % line)

            if state > 0:
                # flush out old section
                # biglm library expects means, not boundaries
                # so we give the median of each quantile, which is not quite right
                if count[state] == 0:
                    sys.stderr.write("warning: no %d-grams, skipping\n" % state)
                else:
                    sys.stdout.write("prob%d %s\n" % (state, " ".join(str(qprob[(i+0.5)/opts.prob_quanta]) for i in xrange(opts.prob_quanta))))
                    if state < order:
                        sys.stdout.write("bow%d %s\n" % (state, " ".join(str(qbow[(i+0.5)/opts.bow_quanta]) for i in xrange(opts.bow_quanta))))


            if m is not None:
                state = int(m.group(1))
                sys.stderr.write("%d-grams\n" % state)
                if count[state] > 0:
                    qprob = manku.Quantiler(count[state], eps=0.01)
                    qbow = manku.Quantiler(count[state], eps=0.01)
        elif state > 0:
            fields = line.split()
            if len(fields) > 0:
                ngram = tuple(fields[1:1+state])
                prob = float(fields[0])
                qprob.append(prob)
                if state < order:
                    try:
                        bow = float(fields[1+state])
                    except IndexError:
                        bow = 0.
                    qbow.append(bow)
        elif state == 0:
            m = countre.match(line)
            if m is not None:
                o = int(m.group(1))
                count[o] = int(m.group(2))
                order = max(order, o)

