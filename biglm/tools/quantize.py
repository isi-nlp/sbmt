#!/usr/bin/env python

import sys
import re
import math
import bisect
import random
import collections

class Quantizer(object):
    def __init__(self, n=None, means=None):
        self.n = n
        self.means = means
        if means is not None:
            self.n = len(means)
            self._update()

    def _update(self):
        self.means.sort()
        self.boundaries = [0.5*(self.means[i+1]+self.means[i]) for i in xrange(self.n-1)]

    def train(self, xs, threshold=0.0001):
        sys.stderr.write("training on %d values\n" %  len(xs))
        # initialize to random intervals
        self.means = random.sample(xs, self.n)
        self._update()

        error = prev_error = None
        perturbed = False

        while prev_error is None or abs(error-prev_error) > threshold or perturbed:
            sys.stderr.write("means = %s\n" % " ".join("%.2f" % x for x in self.means))
        
            # find center of mass of each interval
            sums = [0.]*self.n
            counts = [0]*self.n

            prev_error = error
            error = 0.
            for x in xs:
                i = self.quantize(x)
                sums[i] += x
                counts[i] += 1

                error += (x-self.means[i])*(x-self.means[i])

            for i in xrange(self.n):
                # if there is an empty bin, reset its mean to a random number
                if counts[i] == 0:
                    if i > 0:
                        b1 = "%.2f" % self.boundaries[i-1]
                    else:
                        b1 = "-inf"
                    if i < self.n-1:
                        b2 = "%.2f" % self.boundaries[i]
                    else:
                        b2 = "inf"
                    sys.stderr.write("empty bin: (%s,%s)\n" % (b1,b2))

                    self.means[i] = random.sample(xs, 1)[0]
                    perturbed = True
                else:
                    self.means[i] = sums[i]/counts[i]
                    perturbed = False

            self._update()
            error = math.sqrt(error/sum(counts))

            sys.stderr.write("error: %f\n" % error)
            
    def quantize(self, x):
        return bisect.bisect(self.boundaries, x)

    def estimate(self, i):
        return self.means[i]

class Vocab(object):
    def __init__(self):
        self.ids = {}

    def __getitem__(self, w):
        return self.ids.setdefault(w, len(self.ids))

    def __contains__(self, w):
        return w in self.ids

def read_ngrams(filename, order):
    sys.stderr.write("reading %d-grams from %s" % (order,filename))
    progress = 0
    ngrams = [{} for o in xrange(order)]
    for line in file(filename):
        words = ["<s>"]
        words.extend(line.split())
        words.append("</s>")
        for o in xrange(order):
            for i in xrange(0,len(words)-o):
                ngram = tuple(words[i:i+o+1])
                if o == order-1:
                    if ngram in ngrams[o]:
                        ngrams[o][ngram][0] += 1
                    else:
                        ngrams[o][ngram] = [1, None, 0, None]
                else:
                    ngrams[o][ngram] = [0, None, 0, None]
                progress += 1
                if progress % 10000 == 0:
                    sys.stderr.write(".")
    sys.stderr.write("\n")
    ngrams[0][("<unk>",)] = [0, None, 0, None]
    ngrams[0][("<s>",)] = [0, None, 0, None] # reset to 0 in case opts.order == 1
    return ngrams

def scan_lm(filename, order, ngrams):
    sectionre = re.compile(r"\\(\d+)-grams:")

    state = 0
    progress = 0
    for line in file(filename):
        if line.startswith("\\"):
            m = sectionre.match(line)
            if m is not None:
                state = int(m.group(1))
                ngramo = ngrams[state-1]
                sys.stderr.write("%d-grams\n" % state)
        elif state > 0:
            fields = line.split()
            ngram = tuple(fields[1:1+state])
            values = ngramo.get(ngram, None)
            if values is not None:
                prob = float(fields[0])
                values[1] = prob
                if state < order:
                    try:
                        bow = float(fields[1+state])
                    except IndexError:
                        bow = 0.
                    values[3] = bow
        progress += 1
        if progress % 1000000 == 0:
            sys.stderr.write("%d lines\n" % progress)

    return ngrams

def get_data_from_heldout():
    ngrams = read_ngrams(opts.train, opts.order)
    sys.stderr.write("scanning %s\n" % args[0])
    scan_lm(args[0], opts.order, ngrams)
    
    sys.stderr.write("backoff\n")
    for o in xrange(opts.order-1,0,-1):
        for (ngram, values) in ngrams[o].iteritems():
            if values[1] is None:
                ngrams[o-1][ngram[1:]][0] += values[0]
                ngrams[o-1][ngram[:-1]][2] += values[0]

    # This is not backoff: any word which has no prob is an unknown word
    # and gets rewritten as <unk>, i.e., both its prob and bow lookups should
    # get counted towards <unk>.
    # This does not work correctly for non-unigrams containing unknown words
    # but that's okay because our language models never have them.
    for (ngram, values) in ngrams[0].iteritems():
        if values[1] is None:
            ngrams[0][("<unk>",)][0] += values[0]
            ngrams[0][("<unk>",)][2] += values[2]

    ptrain = {}
    for i in xrange(1, opts.order+1):
        train = []
        for (ngram,values) in ngrams[i-1].iteritems():
            if values[1] is not None:
                train.extend([values[1]]*values[0]) # really dumb
        ptrain[i] = train

    btrain = {}
    for i in xrange(1, opts.order):
        train = []
        for (ngram,values) in ngrams[i-1].iteritems():
            if values[3] is not None:
                train.extend([values[3]]*values[2])

        btrain[i] = train

    return ptrain, btrain

def get_data_uniform():
    sectionre = re.compile(r"\\(\d+)-grams:")
    filename = args[0]

    ptrain = collections.defaultdict(list)
    btrain = collections.defaultdict(list)

    state = 0
    progress = 0
    for line in file(filename):
        if line.startswith("\\"):
            m = sectionre.match(line)
            if m is not None:
                state = int(m.group(1))
                sys.stderr.write("%d-grams\n" % state)
        elif state > 0:
            fields = line.split()
            if len(fields) == 0: continue
            
            ngram = tuple(fields[1:1+state])

            if random.random() < opts.uniform:
                ptrain[state].append(float(fields[0]))
                if state < opts.order:
                    try:
                        bow = float(fields[1+state])
                    except IndexError:
                        bow = 0.
                    btrain[state].append(bow)
        progress += 1
        if progress % 1000000 == 0:
            sys.stderr.write("%d lines\n" % progress)

    return ptrain, btrain

if __name__ == "__main__":
    import sys
    import optparse
    
    import gc
    gc.disable()

    optparser = optparse.OptionParser()
    optparser.add_option("-p", "--prob-quanta", dest="prob_quanta", type=int, help="number of quanta for probabilities")
    optparser.add_option("-b", "--bow-quanta", dest="bow_quanta", type=int, help="number of quanta for backoff weights")
    optparser.add_option("-P", "--prob-bits", dest="prob_bits", type=int, help="number of bits for probabilities")
    optparser.add_option("-B", "--bow-bits", dest="bow_bits", type=int, help="number of bits for backoff weights")
    optparser.add_option("-n", "--order", dest="order", type=int, help="order of language model (required)")
    optparser.add_option("-e", "--threshold", dest="threshold", help="stopping threshold", type=float, default=0.0001)
    optparser.add_option("-t", dest="train", type=str, help="heldout data")
    optparser.add_option("-u", dest="uniform", type=float, help="sample uniformly from LM")
    (opts, args) = optparser.parse_args()

    if opts.prob_quanta is None:
        opts.prob_quanta = 2**opts.prob_bits
    if opts.bow_quanta is None:
        opts.bow_quanta = 2**opts.bow_bits

    if opts.train:
        ptrain, btrain = get_data_from_heldout()
    elif opts.uniform:
        ptrain, btrain = get_data_uniform()
    else:
        sys.stderr.write("You must use -t or -u.\n")
        sys.exit(1)

    sys.stderr.write("quantizing\n")
    pq = Quantizer(n=opts.prob_quanta)
    bq = Quantizer(n=opts.bow_quanta)

    for i in xrange(1,opts.order+1):
        pq.train(ptrain[i], threshold=opts.threshold)
        sys.stdout.write("prob%d %s\n" % (i, " ".join(str(b) for b in pq.means)))

        if i < opts.order:
            bq.train(btrain[i], threshold=opts.threshold)
            sys.stdout.write("bow%d %s\n" % (i, " ".join(str(b) for b in bq.means)))

