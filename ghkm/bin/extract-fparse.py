#!/usr/bin/env python

import sys
import re
import itertools
import optparse
import subprocess
import tree

# Input: eparse \t fparse \t align
# eparse is in Radu format
# fparse is in PTB format
# align is in e-f format

# Output: xRS rules

if __name__ == "__main__":
    for line in sys.stdin:
        etree, ftree, align = line.rstrip().split("\t")
        ftree = tree.Node.from_str(ftree)

        if not ftree:
            sys.stderr.write("warning: skipping line due to failed French parse\n")
            continue

        if ftree.label in ['', 'ROOT', 'TOP']:
            ftree = ftree.children[0]
        
        fwords = " ".join(leaf.label for leaf in ftree.frontier())

        # make an index of the highest node for each span
        spans = {}
        j = 0
        for node in ftree.postorder():
            if node.is_terminal():
                node.span = (j,j+1)
                j += 1
            else:
                node.span = (node.children[0].span[0], node.children[-1].span[1])
                spans[node.span] = [node.label]
        n = j

        # now make a complex category for each span (all pairs shortest paths)
        for l in xrange(2,n):
            for i in xrange(0,n-l+1):
                j = i+l
                for k in xrange(i+1,j):
                    if (i,k) not in spans or (k,j) not in spans: continue
                    if (i,j) not in spans or len(spans[i,k])+len(spans[k,j]) < len(spans[i,j]):
                        spans[i,j] = spans[i,k] + spans[k,j]

        for (i,j) in spans:
            spans[i,j] = "+".join(spans[i,j])

        args = ["-x", "-", "-H"] + sys.argv[1:]
        extract = subprocess.Popen(["/home/nlg-02/pust/ghkm/xrs-extract/bin/extract"]+args, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        rules, _ = extract.communicate("\n".join([etree,fwords,align,""]))

        frhs_re = re.compile(r"-> (.*) ###")
        srcb_re = re.compile(r" srcb={{{ (.*?) }}}")
        span_re = re.compile(r"\[(\d+),(\d+)\]")
        eroot_re = re.compile(r"^(.*?)\(")
        lineNumber_re = re.compile(r" lineNumber=(\d+)")
        var_re = re.compile(r"x\d+$")

        for line in rules.split("\n"):
            line = line.rstrip()
            if line == "": continue

            try:
                frhs = frhs_re.search(line).group(1).split() # assume no spaces in tokens!
                srcb = [(int(i),int(j)) for (i,j) in span_re.findall(srcb_re.search(line).group(1))]
                lineNumber = int(lineNumber_re.search(line).group(1))-1
                eroot = eroot_re.search(line).group(1)
            except:
                sys.stderr.write("couldn't process rule: %s\n" % line)
                print line
                raise

            fvars = []
            froot = spans.get((srcb[0][0],srcb[-1][1]), None)
            for f, span in itertools.izip(frhs, srcb):
                if var_re.match(f):
                    fvars.append("%s" % spans.get(span, None))

            print line, "froot=%s fvars={{{%s}}} fesyn={{{%s %s}}}" % (froot, " ".join(fvars), froot, eroot)
