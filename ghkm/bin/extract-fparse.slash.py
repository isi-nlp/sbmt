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

class Category(object):
    def __init__(self, base, largs=None, rargs=None):
        self.base = tuple(base)
        self.largs = tuple(largs or [])
        self.rargs = tuple(rargs or [])

    def __len__(self):
        return len(self.base) + len(self.largs) + len(self.rargs)

    def __str__(self):
        s = "+".join(self.base)
        s = "\\".join(self.largs+(s,))
        s = "/".join((s,)+self.rargs)
        return s

    def __eq__(self, other):
        return isinstance(other, Category) and self.base == other.base and self.largs == other.largs and self.rargs == other.rargs

    def __hash__(self):
        return hash((self.base,self.largs,self.rargs))


if __name__ == "__main__":
    for line in sys.stdin:
        etree, ftree, align = line.rstrip().split("\t")
        ftree = tree.Node.from_str(ftree)

        if not ftree:
            sys.stderr.write("warning: skipping line due to failed French parse\n")
            continue

        if ftree.label in ['', 'ROOT', 'TOP']:
            ftree = ftree.children[0]
            ftree.detach()
        
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
                spans[node.span] = node.label
        n = j

        # now make a complex category for each span (all pairs shortest paths)
        prod = {}
        for i,j in spans:
            prod[i,j] = [spans[i,j]]
        
        for l in xrange(2,n):
            for i in xrange(0,n-l+1):
                j = i+l
                for k in xrange(i+1,j):
                    if (i,k) not in prod or (k,j) not in prod: continue
                    if (i,j) not in prod or len(prod[i,k])+len(prod[k,j]) < len(prod[i,j]):
                        prod[i,j] = prod[i,k] + prod[k,j]

        cats = {}
        for i,j in prod:
            cats[i,j] = Category(prod[i,j])
        
        # slash categories
            
        for i,j in prod:
            # find smallest containing node
            node = ftree
            while not node.is_terminal():
                for child in node.children:
                    ci,cj = child.span
                    if ci <= i and j <= cj:
                        node = child
                        break
                else:
                    break
            # but we want the highest in a unary chain
            while node.parent and len(node.parent.children) == 1:
                node = node.parent

            (ni,nj) = node.span
            if (ni,i) in prod and (j,nj) in prod:
                cat = Category([node.label], prod[ni,i], prod[j,nj])
                if (i,j) not in cats or len(cat) <= len(cats[i,j]):
                    cats[i,j] = cat

        for (i,j) in cats:
            cats[i,j] = str(cats[i,j])

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
            froot = cats.get((srcb[0][0],srcb[-1][1]), None)
            for f, span in itertools.izip(frhs, srcb):
                if var_re.match(f):
                    fvars.append("%s" % cats.get(span, None))

            print line, "froot=%s fvars={{{%s}}} fesyn={{{%s %s}}}" % (froot, " ".join(fvars), froot, eroot)
