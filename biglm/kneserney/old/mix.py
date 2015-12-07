#!/usr/bin/env python

import heapq, sys, itertools

def imerge(*iters):
    h = []
    for it in iters:
        it = iter(it)
        try:
            h.append((it.next(), it))
        except StopIteration:
            pass

    heapq.heapify(h)

    while len(h) > 0:
        (x, it) = heapq.heappop(h)
        yield x
        try:
            heapq.heappush(h, (it.next(), it))
        except StopIteration:
            pass

def read(filename):
    for line in file(filename):
        p, rest = line.strip().split(None, 1)
        p = float(p)
        yield rest, p

def scale(file, weight):
    for rest, p in file:
        p *= weight
        yield rest, p

def coalesce(file):
    prev = None
    s = 0.
    for cur, p in file:
        if prev is not None and cur != prev:
            yield prev, s
            s = 0.
            
        s += p
        prev = cur

    if prev is not None:
        yield prev, s

if __name__ == "__main__":
    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option("-w", "--weights", dest="weights")
    (opts, args) = optparser.parse_args()

    if opts.weights:
        weights = [float(w) for w in opts.weights.split()]
    else:
        weights = []

    if len(args) > len(weights):
        w = (1.-sum(weights)) / (len(args)-len(weights))
        while len(weights) < len(args):
            weights.append(w)
    elif len(args) < len(weights):
        sys.stderr.write("warning: more weights than inputs\n")
        weights = weights[:len(args)]

    inputs = zip(args, weights)

    sys.stderr.write("mixing:\n")
    for filename, weight in inputs:
        sys.stderr.write("  %s\t* %s\n" % (filename, weight))

    if sum(weights) < 1.:
        sys.stderr.write("warning: weights sum to %s\n" % sum(weights))

    for rest,p in coalesce(imerge(*[scale(read(filename), weight) for (filename, weight) in inputs])):
        sys.stdout.write("%s\t%s\n" % (p, rest))
        
