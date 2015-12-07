#!/usr/bin/env python

# usage:

# Perform a join operation on the given streams to form a single
# stream.  In each input stream, the first <key-fields> tab-delimited
# fields are taken to be the key. For each key, the cartesian product
# of the sets of values is output.

# The first input is special in that it can have very large value
# sets. All the other inputs should have value sets small enough that
# their cartesian product can fit into memory.

# Example: given a set of rules and two functions x(rule) and y(rule), calculate P(y|x)

# map    rule, count(rule) -> x, y, count(rule)
# reduce                   -> x, y, count(x,y)

# map    x, y, count(x,y) -> x, count(x,y)
# reduce                  -> x, count(x)

# join   x, y, count(x,y) and x, count(x) -> x, y, count(x,y), count(x)
# reduce                                  -> x, y, P(y|x)

# map    rule -> x, y, rule

# join   x, y, rule and x, y, P(y|x) -> x, y, rule, P(y|x)
# reduce                             -> rule, P(y|x)

# how it works:

# number the input sources such that the big input is the last one
# map key value -> key i value where i is the input source
# partition using key
# sort using key, i
# reduce: for each key, collect non-big inputs and form cartesian product.
#   then for each big input, paste all products.

# options:
#   -f ignore when a small input has a key not present in large input
#   -e supply single empty value for a missing value
#   -r <reducer>
#   -k <key-fields>
#   -o <output>

import sys, os, subprocess
import os.path, glob, tempfile
import math
import getopt
import itertools
import urlparse
urlparse.uses_relative.append('hdfs')
urlparse.uses_netloc.append('hdfs')

def crossproduct(*xs):
    if len(xs) == 0:
        yield []
        return
    elif len(xs) == 1:
        for x in xs[0]:
            yield x
        return
    else:
        for x in xs[0]:
            for ys in crossproduct(*xs[1:]):
                yield x+ys

if __name__ == "__main__":
    opts, args = getopt.gnu_getopt(sys.argv[1:], 'MRk:o:r:fe')
    opts = dict(opts)

    n_keys = int(opts.get("-k", 1))
    n_sources = len(args)
    
    if "-M" in opts: # Map stage
        # Get list of absolute URLs for sources. We could do
        # sourcepaths = os.environ['mapred_input_dir'].split(',')
        # but I want to make sure that the order is preserved

        cwd = os.environ['mapred_working_dir']
        if cwd[-1] != '/':
            cwd = cwd + '/'
        sourcepaths = [urlparse.urljoin(cwd, arg) for arg in args]

        # Figure out which input path our input file came from.

        sourcefile = os.environ['map_input_file']
        source = None
        for i, sourcepath in enumerate(sourcepaths):
            if sourcefile.startswith(sourcepath):
                assert source is None
                source = i
        assert source is not None

        # Tag every record with what input path it came from.
        # We rotate the sources so that the first source (the big one) is last.

        source = (source-1) % n_sources
        digits = int(math.ceil(math.log10(n_sources)))
        source = "%0*d" % (digits, source)

        for line in sys.stdin:
            fields = line.rstrip().split("\t")
            fields[n_keys:n_keys] = [source]
            print "\t".join(fields)

    elif "-R" in opts: # Reduce stage

        def input():
            for line in sys.stdin:
                yield line.rstrip().split("\t")
        
        for key, records1 in itertools.groupby(input(), lambda fields: fields[:n_keys]):
            values = []
            done = False
            for source, records in itertools.groupby(records1, lambda fields: fields[n_keys]):
                source = int(source)

                if source < n_sources-1: # little sources
                    assert not done
                    while source > len(values)-1:
                        values.append([])

                    for fields in records:
                        value = fields[n_keys+1:]
                        values[source].append(value)

                else: # big source
                    if '-e' in opts:
                        for vs in values:
                            if len(vs) == 0:
                                vs.append("")
                    values = list(crossproduct(*values))
                    for fields in records:
                        big_value = fields[n_keys+1:]
                        for value in values:
                            print "\t".join(key+big_value+value)
                    done = True

            if not done:
                if "-f" not in opts:
                    raise Exception("no value from first source for key %s\n" % key)
                if "-e" in opts:
                    for vs in values:
                        if len(vs) == 0:
                            vs.append("")
                    for value in crossproduct(*values):
                        print "\t".join(key+[""]+value)
                    

    else: # Top-level mode

        outputfile = opts['-o']
        
        hadoop_home = os.environ['HADOOP_HOME']
        hadoop = os.path.join(hadoop_home, "bin/hadoop")
        hadoop_streaming = glob.glob(os.path.join(hadoop_home, "contrib/streaming/hadoop-*-streaming.jar"))[0]
        me = "%s %s" % (sys.executable, os.path.abspath(sys.argv[0]))

        command = [hadoop, "jar", hadoop_streaming]

        for sourcepath in args:
            command.extend(["-input", sourcepath])

        mapper = "%s -M -k %s %s" % (me, n_keys, " ".join(args))
        command.extend(["-mapper", mapper])

        # Sort on n_keys+1 fields, but partition on n_keys fields.
        # That is, each reducer gets all the records with the same real key,
        # but sorted according to the source tag that we added
        command.extend(["-jobconf", "stream.num.map.output.key.fields=%d" % (n_keys+1),
                        "-partitioner", "org.apache.hadoop.mapred.lib.KeyFieldBasedPartitioner",
                        "-jobconf", "num.key.fields.for.partition=%d" % n_keys])
        
        reducer = [me, "-R"]
        reducer.extend(["-k", str(n_keys)])
        if "-f" in opts: reducer.append("-f")
        if "-e" in opts: reducer.append("-e")
        reducer.extend(args)
        reducer = " ".join(reducer)
        if "-r" in opts:
            # create a mini-script to avoid quotes-within-quotes problem
            reducerfile = tempfile.NamedTemporaryFile()
            reducerfile.write("#!/bin/sh\n")
            reducerfile.write("%s | %s\n" % (reducer, opts["-r"]))
            reducerfile.flush()
            reducer = os.path.basename(reducerfile.name)
            command.extend(["-file", reducerfile.name])
        command.extend(["-reducer", reducer])

        command.extend(["-output", outputfile])
        
        subprocess.check_call(command)

