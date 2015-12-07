#!/nfshomes/dchiang/pkg/python/bin/python

# alignment.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

import sys, re
import itertools
import sym

class Alignment(object):
    def __init__(self, fwords, ewords, comment=None):
        m = len(fwords)
        n = len(ewords)
        self.aligned = [[0 for j in xrange(n)] for i in xrange(m)]
        self.faligned = [0]*m
        self.ealigned = [0]*n
        self.fwords = fwords
        self.ewords = ewords
        self.comment = comment
        
    eline_re = re.compile(r"([^\s]+)\s+\(\{\s+((?:\d+\s+)*)\}\)")
    def reader(file, transpose=False):
        while True:
            try:
                comment = file.next().rstrip()
                ewords = [sym.fromstring(word) for word in file.next().split()]
                fline = file.next()
                fxwords = Alignment.eline_re.findall(fline)
            except StopIteration:
                return

            (fword, eindices) = fxwords[0]
            if fword == "NULL":
                fxwords = fxwords[1:]
            fxwords = [(sym.fromstring(fword), eindices) for (fword, eindices) in fxwords]
            fwords = [fword for (fword, eindices) in fxwords]

            if not transpose:
                a = Alignment(fwords, ewords, comment)
            else:
                a = Alignment(ewords, fwords, comment)

            for i in xrange(len(fxwords)):
                (fword, eindices) = fxwords[i]
                for eindex in eindices.split():
                    j = int(eindex)-1
                    if not transpose:
                        a.align(i,j)
                    else:
                        a.align(j,i)

            yield a
    reader = staticmethod(reader)

    aline_re = re.compile(r"(\d+)-(\d+)")
    def reader_pharaoh(ffile, efile, afile):
        progress = 0
        for (fline, eline, aline) in itertools.izip(ffile, efile, afile):
            progress += 1
            fwords = [sym.fromstring(w) for w in fline.split()]
            ewords = [sym.fromstring(w) for w in eline.split()]
            a = Alignment(fwords, ewords)
            for (i,j) in Alignment.aline_re.findall(aline):
                i = int(i)
                j = int(j)
                if i >= len(fwords) or j >= len(ewords):
                    sys.stderr.write("warning, line %d: alignment point (%s,%s) out of bounds (%s,%s)\n" % (progress, i,j,len(fwords),len(ewords)))
                    continue
                a.align(i,j)
            yield a
    reader_pharaoh = staticmethod(reader_pharaoh)

    def write(self, file):
        '''Write in GIZA++ format'''
        file.write("%s\n" % self.comment)
        file.write("%s\n" % " ".join([sym.tostring(word) for word in self.ewords]))
        output = []
        output += ['NULL','({']+[str(j+1) for j in xrange(len(self.ewords)) if not self.ealigned[j]]+['})']
        for i in xrange(len(self.fwords)):
            output += [sym.tostring(self.fwords[i]),'({']+[str(j+1) for j in xrange(len(self.aligned[i])) if self.aligned[i][j]]+['})']
        file.write("%s\n" % " ".join(output))

    def write_pharaoh(self, file):
        '''Write in Pharaoh format'''
        output = []
        for i in xrange(len(self.fwords)):
            for j in xrange(len(self.ewords)):
                if self.aligned[i][j]:
                    output += ['%d-%d' % (i,j)]
        file.write("%s\n" % " ".join(output))

    def write_visual(self, file):
        file.write(" ")
        for j in xrange(len(self.ewords)):
            file.write("%d" % (j % 10))
        file.write("\n")
        for i in xrange(len(self.fwords)):
            file.write("%d" % (i % 10))
            for j in xrange(len(self.ewords)):
                if self.aligned[i][j]:
                    file.write("*")
                else:
                    file.write(".")
            file.write("\n")

    def align(self, i, j):
        if not self.aligned[i][j]:
            self.aligned[i][j] = 1
            self.faligned[i] = 1
            self.ealigned[j] = 1

    def intersect(a1, a2):
        a = Alignment(a1.fwords, a1.ewords)
        for i in xrange(len(a.fwords)):
            for j in xrange(len(a.ewords)):
                if a1.aligned[i][j] and a2.aligned[i][j]:
                    a.align(i,j)
        return a
    intersect = staticmethod(intersect)

    def union(a1, a2):
        a = Alignment(a1.fwords, a1.ewords)
        for i in xrange(len(a.fwords)):
            for j in xrange(len(a.ewords)):
                if a1.aligned[i][j] or a2.aligned[i][j]:
                    a.align(i,j)
        return a
    union = staticmethod(union)


