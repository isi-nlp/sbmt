class Vector(dict):
    def __init__(self, *args):
        dict.__init__(self)
        if len(args) == 1:
            arg = args[0]
            if isinstance(arg, dict):
                for f, v in arg.iteritems():
                    self[f] = v
            elif isinstance(arg, str):
                for tok in arg.split():
                    f, v = tok.rsplit("=", 1)
                    self[f] = float(v)
        elif len(args) == 2:
            f, v = args
            self[f] = float(v)
        elif len(args) > 2:
            raise TypeError()

    def __getitem__(self, k):
        try:
            return dict.__getitem__(self, k)
        except KeyError:
            return 0.

    def copy(self):
        return Vector(self)
    __copy__ = copy

    def __add__(v1, v2):
        return Vector.__iadd__(v1.copy(), v2)
    def __iadd__(v1, v2):
        for feature, value in v2.iteritems():
            v1[feature] += value
        return v1

    def __sub__(v1, v2):
        return Vector.__isub__(v1.copy(), v2)
    def __isub__(v1, v2):
        for feature, value in v2.iteritems():
            v1[feature] -= value
        return v1

    def __imul__(v, other):
        if isinstance(other, Vector): # Hadamard product
            zero = []
            for feature in v:
                if feature in other:
                    v[feature] *= other[feature]
                else:
                    zero.append(feature)
            for feature in zero:
                del v[feature]
        else: # scalar product
            for feature in v:
                v[feature] *= other
        return v
    def __mul__(v, other):
        return Vector.__imul__(v.copy(), other)
    def __rmul__(v, k):
        return Vector.__imul__(v.copy(), k)
    def __neg__(v):
        return v*-1.

    def __idiv__(v, k):
        for feature in v:
            v[feature] /= k
        return v
    def __div__(v, k):
        return Vector.__idiv__(v.copy(), k)
    __itruediv__ = __idiv__
    __truediv__ = __div__

    def dot(v1, v2):
        dot = 0.
        if len(v1) < len(v2):
            for feature, value in v1.iteritems():
                if feature in v2:
                    dot += value * v2[feature]
        else:
            for feature, value in v2.iteritems():
                if feature in v1:
                    dot += value * v1[feature]
        return dot
    def normsquared(self):
        dot = 0.
        for feature, value in self.iteritems():
            dot += value * value
        return dot
    squarednorm = normsquared
    def norm(self):
        return self.normsquared()**0.5

    def compact(self):
        zeros = []
        for feature, value in self.iteritems():
            if value == 0.:
                zeros.append(feature)
        for feature in zeros:
            del self[feature]

    def __str__(self):
        return " ".join("%s=%s" % (f,v) for (f,v) in self.iteritems())
    def __repr__(self):
        return "Vector(%s)" % dict.__repr__(self)
    def __reduce__(self):
        return (Vector, (), None, None, self.iteritems())
