# lex.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

# this could be made more transparent by simply ensuring that we have
# reference identity between equal strings, and returning the string
# reference

use_srilm = 1

if use_srilm:
    try:
        #import clm as srilm
        import srilm
    except ImportError:
        use_srilm = 0

w2i = {}
i2w = []

def index(word):
    if type(word) is int:
        return word
    if use_srilm:
        return srilm.index(word)
    else:
        index = w2i.get(word)
        if index is not None:
            return w2i[word]
        else:
            i2w.append(word)
            index = len(i2w)-1
            w2i[word] = index
            return index

def word(index):
    if type(index) is str:
        return index
    if use_srilm:
        return srilm.word(index)
    else:
        return i2w[index]

def write(f):
    if use_srilm:
        raise UnsupportedOperationException, "lex.write not supported with SRI-LM"
    for (w,i) in w2i.iteritems():
        f.write("%s\t%d\n" % (w,i))

def read(f):
    if use_srilm:
        raise UnsupportedOperationException, "lex.read not supported with SRI-LM"
    for line in f:
        (w,i) = line.split()
        i = int(i)
        w2i[w] = i
        if i > len(i2w)-1:
            i2w.extend([None]*(i-len(i2w)+1))
        i2w[i] = w
