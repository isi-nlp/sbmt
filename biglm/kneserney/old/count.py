#!/usr/bin/env python

import sys, os, fileinput, tempfile
import heapq
import optparse

import myhash

def metric_suffix(s):
    s = s.strip()
    if s.endswith("k"):
        return int(s[:-1])*1e3
    if s.endswith("M"):
        return int(s[:-1])*1e6
    elif s.endswith("G"):
        return int(s[:-1])*1e9
    else:
        return int(s)

def process_line(line):
    line = line.rstrip("\n")
    count, key = line.split(None,1)
    count = int(count)
    return key, count

def generate_lines(lines):
    progress = 0
    for line in lines:
        words = line.split()
        if opts.startstop:
            words = ["<s>"]+words+["</s>"]
        yield words
        progress += 1
        if opts.verbose and progress % 1000000 == 0:
            sys.stderr.write("%d lines (memory=%s)\n" % (progress, monitor.memory()))

def generate_ngrams(lines, n):
    for words in lines:
        for i in xrange(len(words)-n+1):
            yield words[i:i+n]

def generate_fields(lines, start, stop):
    for words in lines:
        if stop <= len(words):
            yield words[start:stop]

def imerge(*iterables):
    heap = []
    for it in iterables:
        it = iter(it)
        try:
            heap.append((it.next(), it))
        except StopIteration:
            pass
    heapq.heapify(heap)
    while len(heap) > 0:
        x, it = heapq.heappop(heap)
        try:
            heapq.heappush(heap, (it.next(), it))
        except StopIteration:
            pass
        yield x

if __name__ == "__main__":
    import gc
    gc.disable()

    import monitor
    
    optparser = optparse.OptionParser()
    optparser.add_option("-n", "--n-grams", dest="ngrams", help="count n-grams instead of lines", type=int)
    optparser.add_option("-f", "--field", dest="field", help="count field(s) instead of whole lines")
    optparser.add_option("-s", "--startstop", dest="startstop", action="store_true")

    optparser.add_option("-k", "--key", dest="key", help="use field(s) as key for grouping")
    optparser.add_option("-p", "--parallel", dest="parallel")

    optparser.add_option("-c", "--consecutive", dest="consecutive", action="store_true")
    optparser.add_option("-m", "--merge", dest="merge", action="store_true")

    optparser.add_option("-o", "--output-file", dest="output_filename")

    optparser.add_option("-M", "--max-types", dest="max_types", help="maximum number of types to store in memory (allowable suffixes: k, M, G)", default="10M")
    optparser.add_option("-v", "--verbose", dest="verbose", action="store_true")
    
    (opts, args) = optparser.parse_args()

    # prepare command-line arguments
    if opts.parallel:
        if opts.consecutive or opts.merge:
            raise ValueError("-c and -m not supported with -p")
        (residue, modulus) = opts.parallel.split(":")
        residue = int(residue)
        modulus = int(modulus)
        sys.stderr.write("I am %d of %d\n" % (residue, modulus))

        # rotate args to reduce load on filesystem
        args = [args[(i+residue)%len(args)] for i in xrange(len(args))]

        if opts.key:
            cols = [int(x) for x in opts.key.split("-")]
            if len(cols) == 1:
                keystart, keystop = cols[0]-1, cols[0]
            elif len(cols) == 2:
                keystart, keystop = cols[0]-1, cols[1]
            else:
                parser.print_help()
                sys.exit(1)

    opts.max_types = metric_suffix(opts.max_types)

    if opts.output_filename:
        outfile = file(opts.output_filename, "w")
    else:
        outfile = sys.stdout

    # prepare input
    if not opts.merge:
        input = generate_lines(fileinput.input(args))
        if opts.ngrams is not None:
            input = generate_ngrams(input, opts.ngrams)
        elif opts.field is not None:
            cols = [int(x) for x in opts.field.split("-")]
            if len(cols) == 1:
                input = generate_fields(input, cols[0]-1, cols[0])
            elif len(cols) == 2:
                input = generate_fields(input, cols[0]-1, cols[1])
            else:
                parser.print_help()
                sys.exit(1)

    if opts.consecutive:
        # In this mode we assume that identical keys are
        # consecutive, just like uniq -c. The output
        # is in the same order as the input.
        count = 0
        prev = None
        for words in input:
            line = " ".join(words)
            if prev is not None and line != prev:
                outfile.write("%s\t%s\n" % (count, prev))
                count = 0
            count += 1
            prev = line
        if prev is not None:
            outfile.write("%s\t%s\n" % (count, prev))
    else:
        if opts.merge:
            files = [file(name) for name in args]
            count = {}
        else:
            # In this mode we don't assume anything about
            # the input. The output is sorted, just like
            # sort | uniq -c.
            count = {}
            files = []
            for words in input:
                line = " ".join(words)
                if opts.key:
                    key = " ".join(words[keystart:keystop])
                else:
                    key = line
                if opts.parallel and myhash.myhash(key,modulus) != residue:
                    continue
                count[line] = count.get(line, 0)+1
                if len(count) >= opts.max_types:
                    if opts.verbose:
                        sys.stderr.write("writing counts to temporary file (memory=%s)\n" % monitor.memory())
                    keys = count.keys()
                    keys.sort()
                    f = tempfile.TemporaryFile()
                    for key in keys:
                        f.write("%s\t%s\n" % (count[key], key))
                    f.seek(0)
                    files.append(f)
                    count = {}
                    del keys

        fileinput.close()
        sys.stderr.write("merging %d files to output (memory=%s)\n" % (len(files), monitor.memory()))

        heap = []
        for f in files:
            line = f.readline()
            if line != "":
                heap.append((process_line(line), f))
        for (key, c) in count.iteritems():
            heap.append(((key,c),None))
        heapq.heapify(heap)

        prevkey = None
        sum = 0
        progress = 0
        while len(heap) > 0:
            ((key, c), f) = heapq.heappop(heap)
            if prevkey is not None and key != prevkey:
                outfile.write("%s\t%s\n" % (sum, prevkey))
                sum = 0
            sum += c
            prevkey = key

            if f is not None:
                line = f.readline()
                if line != "":
                    heapq.heappush(heap, (process_line(line), f))
            progress += 1
            if opts.verbose and progress % 1000000 == 0:
                sys.stderr.write("%d lines\n" % progress)

        if prevkey is not None:
                outfile.write("%s\t%s\n" % (sum, prevkey))
        
    if opts.verbose:
        sys.stderr.write("done.\n")
