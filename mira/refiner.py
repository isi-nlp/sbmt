#!/nfshomes/dchiang/pkg/python/bin/python

# refiner.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

import alignment, sym

"""Some basic predicates for the growing functions.
These are not the greatest names. """

def neighbor_aligned(self, i, j, diag=True):
    ''' ...
        .o*
        ...
    '''
    if diag:
        neighbors = [(-1,-1),(0,-1),(+1,-1),(-1,0),(+1,0),(-1,+1),(0,+1),(+1,+1)]
    else:
        neighbors = [(0,-1),(-1,0),(+1,0),(+1,0)]
    for (di,dj) in neighbors:
        if 0<=i+di<len(self.fwords) and 0<=j+dj<len(self.ewords):
            if self.aligned[i+di][j+dj]:
                return True
    return False

def lneighbor_aligned(self, i, j):
    '''Test to see whether the addition of (i,j) would form an L shape, that is,
    an alignment with both a horizontal and a vertical neighbor.

        .*.    .**
        .o*    .o.
        ...    ...

    '''

    # Any two out of three of these lists will form an L shape
    xneighbors = [[( 0,+1),(+1,+1),(+1, 0)],
                  [(+1, 0),(+1,-1),( 0,-1)],
                  [( 0,-1),(-1,-1),(-1, 0)],
                  [(-1, 0),(-1,+1),( 0,+1)]]
    for neighbors in xneighbors:
        count = 0
        for (di,dj) in neighbors:
            if 0<=i+di<len(self.fwords) and 0<=j+dj<len(self.ewords):
                if self.aligned[i+di][j+dj]:
                    count += 1
        if count >= 2:
            return True
    return False

def rook_aligned(self, i, j):
    '''
        ....
        .o.*
        ....
    '''
    return self.faligned[i] or self.ealigned[j]

def doublerook_aligned(self, i, j):
    '''Holds just in case there are two alignment points (not
    necessarily neighboring) that form an L shape with (i,j)
        .*..
        ....
        .o.*
    '''

    return self.faligned[i] and self.ealigned[j]

def och(self, i, j):
    """Och: 

        *.
        o*   o will not be aligned

        *.
        *o   o will not be aligned

        .*..
        .o.* o will be aligned

        ....
        .o.* o will not be aligned
        ....

        ...
        .o.  o will be aligned
        ...
    """
    return (not rook_aligned(self,i,j) or
            neighbor_aligned(self,i,j) and not lneighbor_aligned(self,i,j))

"""Koehn:

    .*..
    .o.* o will NOT be aligned

    ...
    .o.  o will NOT be aligned
    ...
"""
def koehn(self, i, j):
    """
    'We also always require that a new alignment point connects at least one
    previously unaligned word....we also permit diagonal neighborhood...'
    (Koehn et al., 2003).
    """
    
    return not doublerook_aligned(self,i,j) and neighbor_aligned(self,i,j)

def koehn_final(self, i, j):
    # This implements the 'diag' method
    # ACL 2003 paper says rook_aligned ('diag-and') works best
    #return not doublerook_aligned(self, i,j)
    return not rook_aligned(self, i,j)

def grow(self, pred, idempotent, potential):
    if idempotent:
        for i in xrange(len(self.fwords)):
            for j in xrange(len(self.ewords)):
                if (potential.aligned[i][j] and
                    not self.aligned[i][j] and
                    pred(self, i, j)):
                    self.align(i,j)
    else:
        p = []
        for i in xrange(len(self.fwords)):
            for j in xrange(len(self.ewords)):
                if (potential.aligned[i][j] and
                    not self.aligned[i][j]):
                    p.append((i,j))
        while True:
            flag = False
            for pi in xrange(len(p)):
                (i,j) = p[pi]
                if pred(self, i, j):
                    self.align(i,j)
                    p[pi] = None
                    flag = True
            if not flag:
                break
            p = [x for x in p if x is not None]

if __name__ == "__main__":
    import sys, itertools

    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option('-m', '--method', dest='method', default="grow-diag-final-and", help="refinement method")
    (opts,args) = optparser.parse_args()

    file1 = open(args[0], "r") # f-e.A3.final file
    file2 = open(args[1], "r") # e-f.A3.final file

    count = 1
    for (a1, a2) in itertools.izip(alignment.Alignment.reader(file1, transpose=True),
                                   alignment.Alignment.reader(file2)):

        if len(a1.fwords) != len(a2.fwords) or len(a1.ewords) != len(a2.ewords):
            sys.stderr.write("Length mismatch, sentence %d\n" % count)
            count += 1
            continue

        a = alignment.Alignment.intersect(a1, a2)
        au = alignment.Alignment.union(a1, a2)

        if opts.method=="grow-diag-final-and":
            # Step 1: could use a1 then a2 for potentials, or a2 then a1
            grow(a, koehn, False, au)

            # Step 2: could use a2 then a1; could use rook_aligned instead
            grow(a, koehn_final, True, a1)
            grow(a, koehn_final, True, a2)

        elif opts.method=="och":
            grow(a, och, False, a1, a2)

        elif opts.method=="intersection":
            pass

        elif opts.method == "union":
            a = au

        else:
            sys.stderr.write("Unknown method %s\n" % opts.method)
            raise ValueError

        a.write(sys.stdout)

        #a.write_visual(sys.stderr)
        count += 1

