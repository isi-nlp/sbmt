import model, svector, rule
import log
from ctypes import *

cdll.LoadLibrary('biglmwrap.so')
c_biglm = CDLL('biglmwrap.so')

class c_lm(Structure):
    pass
c_lm_p = POINTER(c_lm)

def declare(f, argtypes, restype=None):
    f.argtypes = argtypes
    f.restype = restype

declare(c_biglm.new_biglm, [c_char_p, c_int, c_float], c_lm_p)
declare(c_biglm.biglm_delete, [c_lm_p])
declare(c_biglm.biglm_lookup_word, [c_lm_p, c_char_p], c_uint)
declare(c_biglm.biglm_wordProb, [c_lm_p, POINTER(c_uint)], c_float)
declare(c_biglm.biglm_wordProbs, [c_lm_p, POINTER(c_uint), c_int, c_int], c_float)
declare(c_biglm.biglm_get_order, [c_lm_p], c_int)

# ctypes in PyPy still seems slower than CPython, so
# we cache results of calling C functions
class cachemethod(object):
    def __init__(self, size=1000):
        self.size = size

    def __call__(self, f):
        cache = {}
        def cached(s, x):
            hx = hash(x) % self.size
            try:
                cx, cy = cache[id(s), hx]
            except KeyError:
                pass
            else:
                if cx == x:
                    return cy
            y = f(s, x)
            cache[id(s), hx] = (x, y)
            return y
        return cached

class Ngram(object):
    def __init__(self, filename, override_unk=None):
        if override_unk is not None:
            self.thisptr = c_biglm.new_biglm(filename, 1, override_unk)
        else:
            self.thisptr = c_biglm.new_biglm(filename, 0, 0.0)
        self._free = c_biglm.biglm_delete # hang on to reference
        self.order = c_biglm.biglm_get_order(self.thisptr)
        self.buf = (c_uint * self.order)()

    def __del__(self):
        self._free(self.thisptr)

    def lookup_word(self, s):
        return c_biglm.biglm_lookup_word(self.thisptr, s)

    #@cachemethod(10000)
    def lookup_ngram(self, words):
        n = len(words)
        self.buf[:n] = words
        return c_biglm.biglm_wordProb(self.thisptr, self.buf, n)

    def lookup_ngrams(self, words, start, stop):
        if start >= stop: return 0.
        n = len(words)
        buf = (c_uint * n)()
        for i in xrange(n):
            if words[i] == HOLE or type(words[i]) is rule.Nonterminal:
                buf[i] = 0 # dummy value
            else:
                buf[i] = words[i]
        return c_biglm.biglm_wordProbs(self.thisptr, buf, n, start, stop)

LOGZERO = -999.0 # BIGLM's is -99, but that shouldn't affect us

HOLE = "<elided>"

class LanguageModel(model.Model):
    def __init__(self, filename, feat, mapdigits=False, p_unk=None):
        model.Model.__init__(self)

        log.write("Reading language model from %s...\n" % filename)
        if p_unk is not None:
            self.ngram = Ngram(filename, override_unk=-p_unk)
        else:
            self.ngram = Ngram(filename)

        self.order = self.ngram.order
        self.mapdigits = mapdigits
        self.unit = svector.Vector(feat, 1.)

        self.START = self.ngram.lookup_word("<s>")
        self.STOP = self.ngram.lookup_word("</s>")

    #@cachemethod(1000)
    def map_word(self, e):
        if type(e) is not str or e == HOLE:
            return e
        else:
            if self.mapdigits:
                e = "".join(self.mapdigits if c.isdigit() else c for c in e)
            return self.ngram.lookup_word(e)

    def make_state(self, enums):
        if self.order == 1:
            return (HOLE,)

        if len(enums) < self.order:
            return tuple(enums)
        else:
            state = []
            state.extend(enums[:self.order-1])
            state.append(HOLE)
            state.extend(enums[-self.order+1:])
            return tuple(state)

    def ngrams(self, ewords):
        """Iterate over all the n-grams in ewords that do not include HOLE or Nonterminals."""
        i, j = 0, 1
        n = len(ewords)
        order = self.order
        while j <= n:
            # Skip over HOLE or Nonterminals
            last = ewords[j-1]
            if last == HOLE or type(last) is rule.Nonterminal:
                i, j = j, j+1
            else:
                yield i, j
                j += 1
                i = max(i, j-order)

    def xngrams(self, ewords):
        n = len(ewords)
        i, j = 0, 1
        while j <= n:
            # Skip over HOLE or Nonterminals
            if j == n or ewords[j] == HOLE or type(ewords[j]) is rule.Nonterminal:
                yield i, j
                i, j = j+1, j+2
            else:
                j += 1

    def transition(self, r, antstates, i, j, j1=None):
        enums = [self.map_word(e) for e in rule.subst(r.erhs, antstates)]
        score = 0.
        for i, j in self.xngrams(enums):
            score += self.ngram.lookup_ngrams(enums, i+self.order-1, j)
        state = self.make_state(enums)
        return (state, self.unit * -score)

    # one of the following two functions is currently broken

    def bonus(self, lhs, state):
        score = self.ngram.lookup_ngrams(state, 0, len(state))
        return self.unit * -score

    def estimate(self, r):
        enums = [self.map_word(e) for e in r.erhs]
        score = 0.
        for i, j in self.xngrams(enums):
            score += self.ngram.lookup_ngrams(enums, i, j)
        return self.unit * -score

    def finaltransition(self, state):
        enums = (self.START,)+state+(self.STOP,)
        score = 0.
        for i, j in self.ngrams(enums):
            if j == 1: continue # don't want bare <s>
            if i == 0 or j-i == self.order:
                ngram_score = self.ngram.lookup_ngram(tuple(enums[i:j]))
                score += ngram_score
        return self.unit * -score

    def strstate(self, state):
        return " ".join(state)

if __name__ == "__main__":
    import fileinput
    lm = Ngram("/scratch/lm1")
    for line in fileinput.input():
        words = ['<s>'] + line.split() + ['</s>']
    totalprob = 0.
    for i in xrange(2,len(words)+1):
        ngram = words[max(0,i-lm.order):i]
        print tuple([lm.lookup_word(w) for w in ngram])
        wordprob = lm.lookup_ngram(tuple([lm.lookup_word(w) for w in ngram]))
        print " ".join(ngram), wordprob
        totalprob += wordprob
    print "total", totalprob
    print tuple([lm.lookup_word(w) for w in words])
    print "new", lm.lookup_ngrams(tuple([lm.lookup_word(w) for w in words]), 1, len(words))
