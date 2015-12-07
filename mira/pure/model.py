# model.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

import math
import svector
import log

zero = svector.Vector()

class Model(object):
    def __init__(self):
        object.__init__(self)
        self.stateless = False # rule cost can be calculated from rule alone. no bonus

    def input(self, input):
        pass

    def bonus(self, x, state):
        return zero

    def estimate(self, r):
        (state, dcost) = self.transition(r, None, None, None)
        return dcost

    def transition(self, rule, antstates, i, j, j1=None):
        return (None, zero)

    def finaltransition(self, state):
        return zero

    def lattice_transition(self, scores):
        # Traverse an edge on the input lattice labeled with f and
        # with feature vector scores
        return zero

    def rescore(self, ewords, score):
        return score

    def strstate(self, state):
        return ""

class WordPenalty(Model):
    def __init__(self, feat):
        Model.__init__(self)
        self.stateless = True
        self.feat = feat

    def transition (self, r, antstates, i, j, j1=None):
        return (None, svector.Vector(self.feat, len(r.erhs)-r.arity()))

    def estimate (self, r):
        return svector.Vector(self.feat, len(r.erhs)-r.arity())

class PhraseModel(Model):
    def __init__(self, srcfeat, feat=None):
        Model.__init__(self)
        self.stateless = True
        self.srcfeat = srcfeat
        self.feat = feat or srcfeat

    def transition(self, r, antstates, i, j, j1=None):
        v = r.scores[self.srcfeat]
        if v != 0.:
            return (None, svector.Vector(self.feat, v))
        else:
            return (None, zero)

    def estimate(self, r):
        v = r.scores[self.srcfeat]
        if v != 0.:
            return svector.Vector(self.feat, v)
        else:
            return zero

class PhrasePenalty(Model):
    def __init__(self, srcfeat, feat=None):
        Model.__init__(self)
        self.stateless = True
        self.srcfeat = srcfeat
        self.feat = feat or srcfeat
        self.fire = svector.Vector(self.feat,1.)
        
    def transition(self, r, antstates, i, j, j1=None):
        if r.scores[self.srcfeat] > 0.:
            return (None, self.fire)
        else:
            return (None, zero)

    def estimate(self, r):
        if r.scores[self.srcfeat] > 0.:
            return self.fire
        else:
            return zero
        
class MultiPhraseModel(Model):
    def __init__(self, feats):
        Model.__init__(self)
        self.stateless = True
        self.feats = feats
        
    def transition(self, r, antstates, i, j, j1=None):
        return (None, r.scores * self.feats)

    def estimate(self, r):
        return r.scores * self.feats

class LatticeModel(Model):
    def __init__(self, feats):
        Model.__init__(self)
        self.feats = feats
        
    def lattice_transition(self, scores):
        return scores * self.feats


        
