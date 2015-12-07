#!/usr/bin/env python

import sys, os, fileinput, tempfile
import optparse

import myhash

if __name__ == "__main__":
    import gc
    gc.disable()

    import monitor
    
    optparser = optparse.OptionParser()
    optparser.add_option("-k", "--key", dest="key", help="use field(s) as key")
    optparser.add_option("-o", "--output-file", dest="output_filename")
    optparser.add_option("-v", "--verbose", dest="verbose", action="store_true")
    optparser.add_option("-p", "--parallel", dest="parallel")

    (opts, args) = optparser.parse_args()

    if opts.output_filename:
        outfile = file(opts.output_filename, "w")
    else:
        outfile = sys.stdout

    (residue, modulus) = opts.parallel.split(":")
    residue = int(residue)
    modulus = int(modulus)
    sys.stderr.write("I am %d of %d\n" % (residue, modulus))

    # rotate args to reduce load on filesystem
    args = [args[(i+residue)%len(args)] for i in xrange(len(args))]

    input = fileinput.input(args)

    if opts.key is not None:
        cols = [int(x) for x in opts.key.split("-")]
        if len(cols) == 1:
            startfield, stopfield = cols[0]-1, cols[0]
        elif len(cols) == 2:
            startfield, stopfield = cols[0]-1, cols[1]
        else:
            parser.print_help()
            sys.exit(1)

    progress = 0
    for line in input:
        if opts.key:
            key = " ".join(line.split()[startfield:stopfield])
        else:
            key = line
        if myhash.myhash(key,modulus) == residue:
            outfile.write(line)
        progress += 1
        if opts.verbose and progress % 1000000 == 0:
            sys.stderr.write("%d lines\n" % progress)
        
        
