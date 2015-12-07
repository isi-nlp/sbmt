import re, svector

class Nonterminal(object):
    nonterminal_re = re.compile(r"""^\[(\S+?)(,(\d+))?\]$""")
    def __init__(self, cat, index=None):
        self.cat = cat
        self.index = index

    def setindex(self, index):
        return Nonterminal(self.cat, index)

    def getindex(self):
        return self.index

    def clearindex(self):
        return Nonterminal(self.cat)

    def __str__(self):
        if self.index is None:
            return "[%s]" % self.cat
        else:
            return "[%s,%s]" % (self.cat,self.index)

    @staticmethod
    def from_str(s):
        if s.startswith('['):
            m = Nonterminal.nonterminal_re.match(s.strip())
            if m:
                if m.group(3):
                    return Nonterminal(m.group(1), int(m.group(3)))
                else:
                    return Nonterminal(m.group(1))
        return s

    def __hash__(self):
        return hash((self.cat, self.index))

    def __eq__(self, other):
        return isinstance(other, Nonterminal) and (self.cat, self.index) == (other.cat, other.index)

class Rule(object):
    def __init__(self, lhs, frhs, erhs):
        self.lhs = lhs
        self.frhs = tuple(frhs)
        self.erhs = tuple(erhs)
        self.scores = svector.Vector()
        self.attrs = Attributes()

    @staticmethod
    def from_str(s):
        fields = s.split(" ||| ")
        lhs = Nonterminal.from_str(fields[0].strip())
        frhs = [Nonterminal.from_str(f) for f in fields[1].split()]
        erhs = [Nonterminal.from_str(e) for e in fields[2].split()]
        r = Rule(lhs, frhs, erhs)
        if len(fields) >= 4:
            r.attrs = Attributes.from_str(fields[3])
        return r

    @staticmethod
    def from_str_hiero(s):
        fields = s.split(" ||| ")
        lhs = Nonterminal.from_str(fields[0].strip())
        frhs = [Nonterminal.from_str(f) for f in fields[1].split()]
        erhs = [Nonterminal.from_str(e) for e in fields[2].split()]
        r = Rule(lhs, frhs, erhs)
        if len(fields) >= 4:
            scores = svector.Vector(fields[3])
        if len(fields) >= 5:
            r.attrs = Attributes()
            r.attrs['align'] = fields[4].strip()
        return r, scores

    def __hash__(self):
        return hash((self.lhs, self.frhs, self.erhs, self.attrs))

    def __eq__(self, other):
        return (self.lhs, self.frhs, self.erhs, self.attrs) == (other.lhs, other.frhs, other.erhs, other.attrs)

    def __str__(self):
        """Rule representation suitable for Hadoop"""
        fields = [str(self.lhs), " ".join(str(f) for f in self.frhs), " ".join(str(e) for e in self.erhs), str(self.attrs)]
        return " ||| ".join(fields)

    def str_hiero(self, scores):
        """Rule representation suitable for Hiero"""
        fields = [str(self.lhs), " ".join(str(f) for f in self.frhs), " ".join(str(e) for e in self.erhs)]
        fields.append(" ".join(str(score) for score in scores))
        if 'align' in self.attrs:
            fields.append(self.attrs['align'])
        return " ||| ".join(fields)

    def str_hiero2(self, scores):
        """Rule representation suitable for Hiero"""
        fields = [str(self.lhs), " ".join(str(f) for f in self.frhs), " ".join(str(e) for e in self.erhs)]
        fields.append(str(scores))
        if 'align' in self.attrs:
            fields.append(self.attrs['align'])
        return " ||| ".join(fields)

    def str_hiero3(self, scores):
        """Rule representation suitable for Hiero"""
        fields = [str(self.lhs), " ".join(str(f) for f in self.frhs), " ".join(str(e) for e in self.erhs), str(scores), str(self.attrs)]
        return " ||| ".join(fields)

    def arity(self):
        return sum(1 for f in self.frhs if isinstance(f, Nonterminal))

class Attributes(dict):
    attributes_re = re.compile("""\s*([^\s=]+)=({{{(.*?)}}}|(\S+))(\s|$)""")

    @staticmethod
    def from_str(s):
        a = Attributes()
        si = 0
        m = Attributes.attributes_re.match(s, si)
        while m:
            a[m.group(1)] = m.group(3) if m.group(3) is not None else m.group(4)
            si = m.end()
            m = Attributes.attributes_re.match(s, si)
        return a

    def __str__(self):
        out = []
        for k, v in sorted(self.iteritems()):
            if type(v) is not str:
                v = str(v)
            if v == "" or " " in v:
                out.append("%s={{{%s}}}" % (k,v))
            else:
                out.append("%s=%s" % (k,v))
        return " ".join(out)

    def __hash__(self):
        return sum(hash(kv) for kv in self.iteritems())

