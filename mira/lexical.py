# lexical.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

"""Model subclass for lexical-selection models"""

import sys, math

import model
import sym
import sgml

def wordpositions(phr, i, j, j1=None):
    result = []
    for c in xrange(phr.arity()+1):
        chunk = phr.getchunk(c)
        # bleh
        if c == 0:
            ci = i
            cj = ci+len(chunk)
        elif c == phr.arity():
            cj = j
            ci = cj-len(chunk)
        else:
            ci = j1
            cj = ci+len(chunk)
        result.extend(range(ci,cj))
        if c < phr.arity():
            result.append(None)
    return result

class LexicalModel(model.Model):
    def __init__(self):
        model.Model.__init__(self)
        self.contextual = True

    def input(self, fwords, meta):
        self.ewords = [{} for f in fwords]
        for (tag, attrs, i, j) in meta:
            attrs = sgml.attrs_to_dict(attrs)
            if attrs.has_key('eword'):
                if j-i != 1:
                    log.write("warning: eword attribute given for multi-word French expression")
                ewords = [sym.fromstring(e.strip()) for e in attrs['eword'].split('|')]
                if 'cost' in attrs:
                    costs = [float(x) for x in attrs['cost'].split('|')]
                elif 'prob' in attrs:
                    costs = [-math.log10(float(x)) for x in attrs['prob'].split('|')]
                else:
                    costs = [-math.log10(1.0/len(ewords)) for e in ewords]
                self.ewords[i] = dict(zip(ewords,costs))

    def estimate(self, r):
        return 0.0

    def transition(self, r, antstates, i, j, j1=None):
        cost = 0.0
        if r.word_alignments is not None:
            w = wordpositions(r.f, i, j, j1)
            for (fi, ei) in r.word_alignments:
                cost += self.ewords[w[fi]].get(r.e[ei], 0.0)
            
        return (None, cost)

class LexicalPenalty(LexicalModel):
    def __init__(self):
        LexicalModel.__init__(self)
        self.alpha = math.log10(math.e)

    def transition(self, r, antstates, i, j, j1=None):
        cost = 0.0
        if r.word_alignments is not None:
            w = wordpositions(r.f, i, j, j1)
            for (fi, ei) in r.word_alignments:
                if r.e[ei] in self.ewords[w[fi]]:
                    cost += self.alpha

        return (None, cost)

class LocalModel1(model.Model):
    """uses p(e|f) only, for now"""
    def __init__(self, ttable, epsilon=1e-40):
        model.Model.__init__(self)
        self.ttable = ttable
        self.epsilon = epsilon # default of 1e-40 from Och et al
        self.stateless = True

    def estimate(self, r):
        total = 0.0
        for e in r.e:
            if not sym.isvar(e):
                subtotal = 0.0
                te = self.ttable.get(e, None)
                if te is None:
                    continue
                l = 0
                for f in r.f:
                    if not sym.isvar(f):
                        subtotal += te.get(f, self.epsilon)
                        l += 1
                subtotal += te.get(None, self.epsilon)
                total += -math.log10(subtotal/(l+1))
        return total

    def transition(self, r, antstates, i, j, j1=None):
        return (None, self.estimate(r))

class IBMModel1(LocalModel1):
    """uses p(e|f): the other direction doesn't really seem possible online"""
    def __init__(self, ttable, epsilon=1e-40):
        LocalModel1.__init__(self, ttable, epsilon)
        self.contextual = True
        self.stateless = False

    def input(self, fwords, meta):
        self.fwords = fwords

    # use LocalModel1 as estimate

    def transition(self, r, antstates, i, j, j1=None):
        total = 0.0
        for e in r.e:
            if not sym.isvar(e):
                subtotal = 0.0
                te = self.ttable.get(e, None)
                if te is None:
                    continue
                for f in self.fwords:
                    subtotal += te.get(f, self.epsilon)
                subtotal += te.get(None, self.epsilon)
                total += -math.log10(subtotal/(len(self.fwords)+1))
        return (None, total)
