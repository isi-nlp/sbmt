# update this to have additive="edge" and additive="sentence" like taylor.py does

import sys, math
import model, svector
import collections, itertools
import sym, log

class Oracle(object):
    def __init__(self, order=4, variant="nist", oracledoc_size=10):
        self.order = order
        
        self.variant = variant.lower()
        if self.variant not in ['ibm','nist','average']:
            raise Exception("unknown BLEU variant %s" % self.variant)
        
        self.oraclemodel = OracleModel(order=order)
        self.wordcounter = WordCounter(variant=self.variant)
        self.models = [self.oraclemodel, self.wordcounter]

        self.oracledoc = svector.Vector("oracle.candlen=1 oracle.reflen=1 oracle.srclen=1")
        for o in xrange(order):
            self.oracledoc["oracle.match%d" % o] = 1
            self.oracledoc["oracle.guess%d" % o] = 1
        self.oracledoc_size = oracledoc_size

        self.feats = list(self.oracledoc)

    def input(self, sent, verbose=True):
        for m in self.models:
            m.input(sent)

        if False and verbose:
            if hasattr(sent, 'fwords'):
                log.write("source:    %s\n" % " ".join(sent.fwords))
            else:
                log.write("sentence %s\n" % sent.id)
            for ref in sent.refs:
                log.write("reference: %s\n" % " ".join(ref))

    def update(self, v):
        """Called at the end of every iteration. v is the finished
           score vector of the 1-best hypothesis, which we can use
           to update our internal state."""

        self.oracledoc += v
        self.oracledoc = (1.-1./self.oracledoc_size)*self.oracledoc
        log.write("new oracle doc: %s; score=%s\n" % (self.oracledoc, self.make_weights().dot(self.oracledoc)))
    
    def make_weights(self, additive=False):
        """Create a special weight vector that can calculate scores.
           If additive=True, try to make the scores approximately
           additive."""
        
        weights = BLEUVector("oracle=1",
                             order=self.order,
                             add=self.oracledoc if additive else None,
                             scale=additive)
        return weights

    def finish(self, v, words):
        """Return a copy of v that contains only the features relevant
        to computing a score. We can also perform any necessary
        corrections to v that are possible knowing the whole
        output."""
        
        # Actually, for BLEU we just recompute from scratch

        # postprocessing: delete non-ASCII chars and @UNKNOWN@
        words = [sym.tostring(w) for w in words]
        words = " ".join(words)
        words = "".join(c for c in words if ord(c) < 128)
        words = [sym.fromstring(word) for word in words.split()]

        v = svector.Vector()

        cand = collections.defaultdict(int)
        for o in xrange(self.order):
            for i in xrange(len(words)-o):
                cand[tuple(words[i:i+o+1])] += 1

        match = collections.defaultdict(int)
        for ngram in cand:
            match[len(ngram)-1] += min(cand[ngram], self.oraclemodel.refngrams[ngram])
        
        for o in xrange(self.order):
            v["oracle.match%d" % o] = match[o]
            v["oracle.guess%d" % o] = max(0,len(words)-o)

        v["oracle.srclen"] = self.wordcounter.srclen
        v["oracle.candlen"] = len(words)
        
        if self.variant == "ibm":
            v["oracle.reflen"] = min((abs(l-len(words)), l) for l in self.wordcounter.reflens)[1]
        else:
            v["oracle.reflen"] = self.wordcounter.reflen

        return v

    def clean(self, v):
        """Return a copy of v that doesn't have any of the features
           used for the oracle."""
        v = svector.Vector(v)
        for f in self.feats:
            del v[f]
        return v

HOLE = sym.fromstring("<elided>")

def make_state(ewords, order):
    if order == 1:
        return (HOLE,)
    elif len(ewords) < order:
        return ewords
    else:
        return ewords[:order-1] + (HOLE,) + ewords[-order+1:]

class OracleModel(model.Model):
    def __init__(self, order=4):
        model.Model.__init__(self)
        self.order = order
        self.feat = ["oracle.match%d" % o for o in xrange(order)]

    def input(self, sent):
        self.refngrams = collections.defaultdict(int)
        for ref in sent.refs:
            ewords = [sym.fromstring(e) for e in ref]
            ngrams = collections.defaultdict(int)
            for o in xrange(self.order):
                for i in xrange(len(ewords)-o):
                    ngrams[tuple(ewords[i:i+o+1])] += 1
            for ngram in ngrams:
                self.refngrams[ngram] = max(self.refngrams[ngram], ngrams[ngram])

    def transition(self, r, antstates, i, j, j1=None):
        match = svector.Vector()
        for o in xrange(1,self.order+1):
            if o == self.order:
                antsubstates = antstates
            else:
                antsubstates = [make_state(antstate, o) for antstate in antstates]
            state = r.e.subst((), antsubstates)

            m = 0
            for i in xrange(len(state)-o+1):
                if (tuple(state[i:i+o]) in self.refngrams):
                    m += 1
            match[self.feat[o-1]] = m

        # state now corresponds to maximum order
        state = make_state(state, self.order)
        
        return (state, match)

    def estimate(self, r):
        if len(r.e)-r.e.arity() == 0:
            return model.zero # a hack to avoid having estimate the glue rule
        
        match = svector.Vector()
        state = r.e.subst((), ((HOLE,),)*r.e.arity())
        for o in xrange(1,self.order+1):
            m = 0
            for i in xrange(len(state)-o+1):
                if (tuple(state[i:i+o]) in self.refngrams):
                    m += 1
            match[self.feat[o-1]] = m

        return match

    def finaltransition(self, state):
        return model.zero

    def strstate(self, state):
        return " ".join(sym.tostring(s) for s in state)
    
class WordCounter(model.Model):
    def __init__(self, variant="nist", stateless=True):
        model.Model.__init__(self)
        self.stateless = stateless
        self.variant = variant

    def input(self, sent):
        # Assume that sent has an flen attr. It doesn't have to be the exact
        # length of the French input sentence (in the case of lattice decoding),
        # but the closer the better.
        self.srclen = sent.flen

        self.reflens = [len(ref) for ref in sent.refs]
        if self.variant == "nist":
            self.reflen = min(self.reflens)
        else: # for IBM, we have to use average
            self.reflen = float(sum(self.reflens))/len(self.reflens)
        #log.write("effective reference length (method %s): %s\n" % (self.variant, self.reflen))

    def transition (self, r, antstates, i, j, j1=None):
        if self.stateless:
            return (None, self.estimate(r))
        else:
            return (sum(antstates)+len(r.e)-r.e.arity(), self.estimate(r))

    def estimate (self, r):
        v = svector.Vector()
        v["oracle.srclen"] = srclen = len(r.f)-r.f.arity()
        v["oracle.candlen"] = candlen = len(r.e)-r.e.arity()
        # pro-rate reference length
        try:
            v["oracle.reflen"] = float(srclen)/self.srclen * self.reflen
        except ZeroDivisionError:
            v["oracle.reflen"] = self.reflen
        return v

class BLEUVector(svector.Vector):
    """A subclass of Vector that can compute scores from
       score vectors when dotted with them."""
    def __init__(self, x, order=4, add=None, scale=True):
        svector.Vector.__init__(self, x)
        self.order = order
        if add is not None:
            self.add = add
        else:
            self.add = svector.Vector() # add zero
        self.matchfeat = ["oracle.match%d" % o for o in xrange(order)]
        self.guessfeat = ["oracle.guess%d" % o for o in xrange(order)]
        self.addmatch = [self.add["oracle.match%d" % o] for o in xrange(order)]
        self.addguess = [self.add["oracle.guess%d" % o] for o in xrange(order)]
        self.addcandlen = self.add["oracle.candlen"]
        self.addreflen = self.add["oracle.reflen"]
        self.addsrclen = self.add["oracle.srclen"]
        self.scale = scale
        
    def dot(self, other):
        model_score = svector.Vector.dot(self, other)

        try:
            precision = 1.
            for o in xrange(self.order):
                m = other[self.matchfeat[o]]+self.addmatch[o]
                if self.guessfeat[o] in other:
                    g = other[self.guessfeat[o]]
                else:
                    # we are scoring a subsentence
                    g = max(0,other["oracle.candlen"]-o)
                g += self.addguess[o]
                precision *= float(m) / g

            precision **= 1./self.order

            reflen = other["oracle.reflen"]
            candlen = other["oracle.candlen"]
            srclen = other["oracle.srclen"]
            brevity = min(1.,math.exp(1.-(reflen+self.addreflen)/(candlen+self.addcandlen)))

            bleu_score = precision*brevity
        except ZeroDivisionError:
            bleu_score = 0.

        if self.scale:
            bleu_score *= self.addcandlen

        #log.write("%s => %s * %s * %s\n" % (other, precision, brevity, candlen+self.addcandlen))

        return model_score + self["oracle"] * bleu_score
