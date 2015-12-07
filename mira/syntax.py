#!/nfshomes/dchiang/pkg/python/bin/python2.4

# syntax.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

# Constituent-reward feature described in ACL 2005 paper

import sys, math
import log
import model, tree, sym

# Could combine Constituent and Constituent1 into single model

PUNC = ["PU"]

class FakeParser:
    '''Reads in a file of parsed sentences and pretends to parse sentences by
    looking them up in the file.'''
    def __init__(self, file):
        self.parses = {}
        for line in file:
            t = tree.str_to_tree(line)
            if t is None:
                log.write("warning: couldn't read tree\n")
                continue
            s = tuple([sym.fromstring(node.label) for node in t.frontier()])
            self.parses[s] = t

    def parse(self, words):
        words = tuple(words)
        if self.parses.has_key(words):
            return self.parses[words]
        else:
            sys.stderr.write("FakeParser.parse(): warning: no parse found\n")
            return None

class SyntaxModel(model.Model):
    def __init__(self, parser, label=None):
        model.Model.__init__(self)
        self.parser = parser
        self.stateless = False
        self.label = label

    def input(self, fwords, meta):
        self.n = len(fwords)

        self.parse = self.parser(fwords)
        self.constituents = [[0] * (self.n+1) for i in xrange(0,self.n+1)]
        if self.parse is not None:
            self.tags = [node.parent.label for node in self.parse.frontier()]
            self.skip_punc = []
            for i in xrange(self.n+1):
                j = i
                while j>0 and self.tags[j-1] in PUNC:
                    j -= 1
                self.skip_punc.append(j)

            self.constituent_helper(self.parse, 0)

            self.constituents = [[self.constituents[self.skip_punc[i]][self.skip_punc[j]] for j in xrange(0,self.n+1)] for i in xrange(0,self.n+1)]
        else:
            self.tags = [None]*len(fwords)
            self.skip_punc = range(self.n+1)

    def constituent_helper(self, node, i):
        if self.label is None or node.label == self.label:
            self.constituents[self.skip_punc[i]][self.skip_punc[i+node.length]] = 1.0
        for child in node.children:
            self.constituent_helper(child, i)
            i += child.length

    def estimate(self, r):
        return 0.0
    
class SynPhraseModel(SyntaxModel):
    def __init__(self, parser, label=None):
        '''Create a model which weights phrases according to various syntactic features.
        parser is a function from word lists to trees.'''
        SyntaxModel.__init__(self, parser, label)
        self.alpha = -math.log10(math.e)

class ConstituentModel(SynPhraseModel):
    def transition (self, rule, antstates, i, j, j1=None):
        if rule not in (grammar.MONOTONE, grammar.STOPMONOTONE): # and j-i > 1:
            #return (None, self.alpha*self.constituents[self.skip_punc[i]][self.skip_punc[j]])
            return (None, self.alpha*self.constituents[i][j])
        else:
            return (None, 0.0)

if __name__ == "__main__":
    p = FakeParser(open("/fs/clip-ssmt/dchiang/parsing/mt02b.parsed", "r"))
