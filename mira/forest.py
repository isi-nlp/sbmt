import sys, math
import itertools, heapq, collections, random
import re, xml.sax.saxutils
import sym, rule, cost, svector, log
import tree

if sys.getrecursionlimit() < 10000:
    sys.setrecursionlimit(10000)

max_deds = 20

def quoteattr(s):
    return '"%s"' % s.replace('\\','\\\\').replace('"', '\\"')

def quotejson(s):
    return '"%s"' % s.replace('\\','\\\\').replace('"', '\\"')

def quotefeature(s):
    return xml.sax.saxutils.escape(s, { ',' : '&comma;' ,
                                        ':' : '&colon;' ,
                                        '=' : '&equals;' ,
                                        ',' : '&comma;' ,
                                        '(' : '&lrb;' ,
                                        ')' : '&rrb;' })

def strstates(models, states):
    return ", ".join(m.strstate(s) for m,s in itertools.izip(models, states))

class Derivation(object):
    def __init__(self, goal):
        self.ded = {}
        self.goal = goal

    def value(self, f, item=None):
        if item is None:
            item = self.goal
        ded = self.ded[id(item)]
        antvalues = [self.value(f, ant) for ant in ded.ants]
        return f(ded, antvalues)

    def french(self):
        return self.value(lambda ded, antvalues: ded.rule.f.subst((), antvalues) if ded.rule else antvalues[0])

    def english(self):
        return self.value(lambda ded, antvalues: ded.rule.e.subst((), antvalues) if ded.rule else antvalues[0])

    def alignment(self):
        def visit(item):
            ded = self.ded[id(item)]
            if ded.rule:
                align = collections.defaultdict(list)
                if 'align' in ded.rule.attrs:
                    for fi, ei in ded.rule.attrs['align']:
                        align[ei].append(fi)

                result = []
                j1 = None
                for ei, e in enumerate(ded.rule.e):
                    if sym.isvar(e):
                        result.extend(visit(ded.ants[sym.getindex(e)-1]))
                    else:
                        if len(ded.ants) == 2:
                            j1 = ded.ants[0].j
                        else:
                            j1 = None
                        result.append([ded.rule.f.stringpos(fi, item.i, item.j, j1) for fi in align[ei]])
                print ded.rule, item.i, item.j, j1, result
                return result
            else:
                return visit(ded.ants[0])

        a = visit(self.goal)
        return {(fi,ei) for (ei,fis) in enumerate(a) for fi in fis}

    @staticmethod
    def _tree_helper(t, antvalues):
        t = tree.str_to_tree(t)
        for node in t.frontier():
            x = sym.fromstring(node.label)
            if sym.isvar(x):
                node.insert_child(0, antvalues[sym.getindex(x)-1])
        return t

    @staticmethod
    def _fake_tree_helper(lhs, rhs, antvalues):
        children = []
        for x in rhs:
            if sym.isvar(x):
                children.append(antvalues[sym.getindex(x)-1])
            else:
                children.append(tree.Node(sym.tostring(x), []))
        return tree.Node(sym.totag(lhs), children)

    def french_tree(self):
        def f(ded, antvalues):
            if ded.rule is None: # goal
                return antvalues[0]
            elif 'ftree' in ded.rule.attrs:
                return Derivation._tree_helper(ded.rule.attrs['ftree'], antvalues)
            else:
                return Derivation._fake_tree_helper(ded.rule.lhs, ded.rule.f, antvalues)
        return self.value(f)

    def english_tree(self):
        def f(ded, antvalues):
            if ded.rule is None: # goal
                return antvalues[0]
            elif 'etree' in ded.rule.attrs:
                return Derivation._tree_helper(ded.rule.attrs['etree'], antvalues)
            else:
                return Derivation._fake_tree_helper(ded.rule.lhs, ded.rule.e, antvalues)
        return self.value(f)

    def vector(self):
        return self.value(lambda ded, antvalues: sum(antvalues, svector.Vector(ded.dcost)))

    def select(self, item, ded):
        self.ded[id(item)] = ded

    def _str_helper(self, item, accum):
        ded = self.ded[id(item)]
        if ded.rule:
            x = ded.rule.lhs
        else:
            x = sym.fromtag("-")
        if len(ded.ants) > 0:
            accum.extend(["(", sym.totag(x)])
            for ant in ded.ants:
                accum.append(" ")
                self._str_helper(ant, accum)
            accum.append(")")
        else:
            accum.append(sym.totag(x))

    def __str__(self):
        accum = []
        self._str_helper(self.goal, accum)
        return "".join(accum)

class NBestInfo(object):
    """Information about an Item that is needed for n-best computation"""
    __slots__ = "nbest", "cands", "index", "english", "ecount"
    def __init__(self, item):
        self.nbest = []    # of (viterbi,ded,antranks)
        self.cands = []    # priority queue of (viterbi,ded,antranks)
        self.index = set() # of (ded,antranks)
        self.english = []
        self.ecount = collections.defaultdict(int)
        for ded in item.deds:
            zeros = (0,)*len(ded.ants)
            self.cands.append((ded.viterbi, ded, zeros))
            self.index.add((ded,zeros))
        heapq.heapify(self.cands)

class NBest(object):
    def __init__(self, goal, ambiguity_limit=None):
        self.goal = goal
        self.nbinfos = {}
        self.ambiguity_limit = ambiguity_limit

    def len_computed(self):
        return len(self.nbinfos[id(self.goal)].nbest)

    def compute_nbest(self, item, n):
        """Assumes that the 1-best has already been found
        and stored in Deduction.viterbi"""

        if id(item) not in self.nbinfos:
            self.nbinfos[id(item)] = NBestInfo(item)
        nb = self.nbinfos[id(item)]

        while len(nb.nbest) < n and len(nb.cands) > 0:
            # Get the next best and add it to the list
            (cost,ded,ranks) = heapq.heappop(nb.cands)

            if self.ambiguity_limit:
                # compute English string
                antes = []
                for ant, rank in itertools.izip (ded.ants, ranks):
                    self.compute_nbest(ant, rank+1)
                    antes.append(self.nbinfos[id(ant)].english[rank])
                if ded.rule is not None:
                    e = ded.rule.e.subst((), antes)
                elif len(antes) == 1: # this is used by the Hiero goal item
                    e = antes[0]

                # don't want more than ambiguity_limit per english
                nb.ecount[e] += 1
                if nb.ecount[e] <= self.ambiguity_limit:
                    nb.nbest.append((cost,ded,ranks))
                    nb.english.append(e)
            else:
                nb.nbest.append((cost,ded,ranks))

            # Replenish the candidate pool
            for ant_i in xrange(len(ded.ants)):
                ant, rank = ded.ants[ant_i], ranks[ant_i]

                if self.compute_nbest(ant, rank+2) >= rank+2:
                    ant_nb = self.nbinfos[id(ant)]
                    nextranks = list(ranks)
                    nextranks[ant_i] += 1
                    nextranks = tuple(nextranks)
                    if (ded, nextranks) not in nb.index:
                        nextcost = cost - ant_nb.nbest[rank][0] + ant_nb.nbest[rank+1][0]
                        heapq.heappush(nb.cands, (nextcost, ded, nextranks))
                        nb.index.add((ded,nextranks))

        return len(nb.nbest)

    def __getitem__(self, i):
        self.compute_nbest(self.goal, i+1)
        return self._getitem_helper(self.goal, i, Derivation(self.goal))

    def _getitem_helper(self, item, i, deriv):
        nb = self.nbinfos[id(item)]
        _, ded, ranks = nb.nbest[i]

        deriv.select(item, ded)

        for ant,rank in itertools.izip(ded.ants, ranks):
            self._getitem_helper(ant, rank, deriv)

        return deriv

class Item(object):
    '''In an and/or graph, this is an or node'''
    __slots__ = "x", "i", "j", "states", "deds", "viterbi"
    def __init__(self, x, i, j, deds=None, states=None, viterbi=None):
        if type(x) is str:
            x = sym.fromstring(x)
        self.x = x
        self.i = i
        self.j = j
        self.deds = deds if deds is not None else []
        self.states = states
        self.viterbi = viterbi

    def __hash__(self):
        return hash((self.x,self.i,self.j,tuple(self.states)))

    def __cmp__(self, other):
        if other is None:
            return 1 # kind of weird
        if self.states == other.states and self.x == other.x and self.i == other.i and self.j == other.j:
            return 0
        return 1

    def __str__(self):
        if self.x is None:
            return "[Goal]"
        else:
            return "[%s,%d,%d,%s,cost=%s]" % (sym.tostring(self.x),self.i,self.j,str(self.states),self.viterbi)

    # this is used by extractor.py
    def derive(self, ants, r, dcost=0.0):
        self.deds.append(Deduction(ants, r, dcost))
        # update viterbi?

    # This actually no longer gets used
    def merge(self, item):
        self.deds.extend(item.deds)

        # best item may have changed, so update score
        if item.viterbi < self.viterbi:
            self.viterbi = item.viterbi

    # Pickling
    def __reduce__(self):
        return (Item, (sym.tostring(self.x), i, j, self.deds))

    # Postorder traversal
    def __iter__(self):
        return self.bottomup()

    def bottomup(self, visited=None):
        if visited is None:
            visited = set()
        if id(self) in visited:
            return
        visited.add(id(self))
        for ded in self.deds:
            for ant in ded.ants:
                for item in ant.bottomup(visited):
                    yield item
        yield self

    def compute_inside(self, weights, insides=None, beta=1.):
        if insides is None:
            insides = {}
        if id(self) in insides:
            return insides
        inside = cost.IMPOSSIBLE
        for ded in self.deds:
            # beta = 0 => uniform
            c = weights.dot(ded.dcost)*beta
            for ant in ded.ants:
                ant.compute_inside(weights, insides)
                c += insides[id(ant)]
            insides[id(ded)] = c
            inside = cost.add(inside, c)
        insides[id(self)] = inside
        return insides

    def compute_outside(self, weights, insides, beta=1.):
        outsides = {}
        outsides[id(self)] = 0.
        topological = list(self.bottomup())
        for item in reversed(topological):
            if id(item) not in outsides:
                # not reachable from top
                outsides[id(item)] = cost.IMPOSSIBLE
                continue
            for ded in item.deds:
                if len(ded.ants) == 0:
                    continue
                # p = Pr(ded)
                p = weights.dot(ded.dcost)*beta + outsides[id(item)]
                for ant in ded.ants:
                    p += insides[id(ant)]
                for ant in ded.ants:
                    if id(ant) not in outsides:
                        outsides[id(ant)] = p-insides[id(ant)]
                    else:
                        outsides[id(ant)] = cost.add(outsides[id(ant)], p-insides[id(ant)])

        return outsides

    def expected_features(self, insides, f=None):
        if f is None:
            f = svector.Vector
        v = {}
        for item in self.bottomup():
            for ded in item.deds:
                v[id(ded)] = f(ded.dcost)
                for ant in ded.ants:
                    v[id(ded)] += v[id(ant)]

                d = cost.prob(insides[id(ded)]-insides[id(item)])*v[id(ded)]
                if id(item) in v:
                    v[id(item)] += d
                else:
                    v[id(item)] = d

        return v

    def expected_product(self, insides, ef, eg):
        ep = {}
        for item in self.bottomup():
            for ded in item.deds:
                ep[id(ded)] = ef[id(ded)] * eg[id(ded)]
                for ant in ded.ants:
                    ep[id(ded)] += ep[id(ant)] - ef[id(ant)] * eg[id(ant)]

                d = cost.prob(insides[id(ded)]-insides[id(item)])*ep[id(ded)]
                if id(item) in ep:
                    ep[id(item)] += d
                else:
                    ep[id(item)] = d

        return ep

    def viterbi_deriv(self, deriv=None, weights=None):
        if deriv is None:
            deriv = Derivation(self)

        viterbi_ded = min((ded.viterbi,ded) for ded in self.deds)[1]
        deriv.select(self, viterbi_ded)
        for ant in viterbi_ded.ants:
            ant.viterbi_deriv(deriv)
        return deriv

    def random_deriv(self, insides, deriv=None):
        if deriv is None:
            deriv = Derivation(self)

        r = random.random()
        p = 0.
        for ded in self.deds:
            p += cost.prob(insides[id(ded)]-insides[id(self)])
            if p > r:
                break
        else: # shouldn't happen
            ded = self.deds[-1]

        deriv.select(self, ded)

        for ant in ded.ants:
            ant.random_deriv(insides, deriv)

        return deriv

    def rescore(self, models, weights, memo=None, add=False, check_states=True):
        """Recompute self.viterbi and self.states according to models
        and weights. Returns the Viterbi vector, and (unlike the
        decoder) only calls weights.dot on vectors of whole
        subderivations, which is handy for overriding weights.dot.

        If add == True, append the new scores instead of replacing the old ones.
        """

        if memo is None:
            memo = {}
        if id(self) in memo:
            return memo[id(self)]

        vviterbi = None
        self.states = None
        for ded in self.deds:
            ded_vviterbi, states = self.rescore_deduction(ded, models, weights, memo, add=add, check_states=check_states)
            if self.states is None:
                self.states = states
            elif check_states and states != self.states:
                # don't check state at the root because we don't care
                log.write("warning: Item.rescore(): id(ded)=%s: inconsistent states %s and %s\n" % (id(ded), strstates(models, states), strstates(models, self.states)))
            if vviterbi is None or ded.viterbi < self.viterbi:
                vviterbi = ded_vviterbi
                self.viterbi = weights.dot(vviterbi)

        memo[id(self)] = vviterbi
        return vviterbi

    def rescore_deduction(self, ded, models, weights, memo, add=False, check_states=True):
        """Recompute ded.dcost and ded.viterbi according to models and weights."""

        vviterbi = svector.Vector()
        for ant in ded.ants:
            vviterbi += ant.rescore(models, weights, memo, add=add, check_states=check_states)

        if not add:
            ded.dcost = svector.Vector()
        states = []
        for m_i in xrange(len(models)):
            antstates = [ant.states[m_i] for ant in ded.ants]
            if ded.rule is not None:
                j1 = ded.ants[0].j if len(ded.ants) == 2 else None
                (state, mdcost) = models[m_i].transition(ded.rule, antstates, self.i, self.j, j1)
            elif len(antstates) == 1: # goal item
                mdcost = models[m_i].finaltransition(antstates[0])
                state = None
            states.append(state)

            ded.dcost += mdcost
        vviterbi += ded.dcost
        ded.viterbi = weights.dot(vviterbi)

        return vviterbi, states

    def reweight(self, weights, memo=None):
        """Recompute self.viterbi according to weights. Returns the
        Viterbi vector, and (unlike the decoder) only calls
        weights.dot on vectors of whole subderivations, which is handy
        for overriding weights.dot."""

        if memo is None:
            memo = {}
        if id(self) in memo:
            return memo[id(self)]

        vviterbi = None
        for ded in self.deds:
            ded_vviterbi = svector.Vector()

            for ant in ded.ants:
                ded_vviterbi += ant.reweight(weights, memo)

            ded_vviterbi += ded.dcost
            ded.viterbi = weights.dot(ded_vviterbi)

            if vviterbi is None or ded.viterbi < self.viterbi:
                vviterbi = ded_vviterbi
                self.viterbi = ded.viterbi

        memo[id(self)] = vviterbi
        return vviterbi

class Deduction(object):
    '''In an and/or graph, this is an and node'''
    __slots__ = "rule", "ants", "dcost", "viterbi"
    def __init__(self, ants, rule, dcost=0.0, viterbi=None):
        self.ants = ants
        self.rule = rule
        self.dcost = dcost
        self.viterbi = viterbi

    def __str__(self):
        return str(self.rule)

    # Pickling
    def __reduce__(self):
        return (Deduction, (self.ants, self.rule, self.dcost))

### Reading/writing forests in ISI format

def forest_to_text(f, mode=None, weights=None):
    result = []
    _item_to_text(f, result, {}, mode=mode, weights=weights)
    return "".join(result)

def _item_to_text(node, result, memo, mode=None, weights=None):
    if id(node) in memo:
        result.append(memo[id(node)])
        return

    nodeid = len(memo)+1
    memo[id(node)] = "#%s" % nodeid

    if len(node.deds) == 1:
        result.append('#%s' % nodeid)
        _ded_to_text(node.deds[0], result, memo, mode=mode, weights=weights)
    else:
        result.append('#%s(OR' % nodeid)

        # keep only the top k deductions to slim the forest
        deds = [(ded.viterbi, ded) for ded in node.deds]
        deds.sort()
        if max_deds:
            deds = deds[:max_deds]
        for _, ded in deds:
            result.append(' ')
            _ded_to_text(ded, result, memo, mode=mode, weights=weights)
        result.append(')')

def _ded_to_text(node, result, memo, mode=None, weights=None):
    # Convert rule and features into single tokens
    #vstr = ",".join("%s:%s" % (quotefeature(f),node.dcost[f]) for f in node.dcost)
    vstr = "cost:%s" % weights.dot(node.dcost)
    #rstr = id(node.rule)
    rstr = id(node)
    s = "%s<%s>" % (rstr,vstr)
    if False and len(node.ants) == 0: # the format allows this but only if we don't tag with an id. but we tag everything with an id
        result.append(s)
    else:
        result.append('(')
        result.append(s)
        if mode == 'french':
            children = node.rule.f if node.rule else node.ants
        elif mode == 'english':
            children = node.rule.e if node.rule else node.ants
        else:
            children = node.ants

        for child in children:
            if isinstance(child, Item):
                result.append(' ')
                _item_to_text(child, result, memo, mode=mode, weights=weights)
            elif sym.isvar(child):
                result.append(' ')
                _item_to_text(node.ants[sym.getindex(child)-1], result, memo, mode=mode, weights=weights)
            else:
                result.append(' ')
                result.append(quoteattr(sym.tostring(child)))
        result.append(')')

class TreeFormatException(Exception):
    pass

dummylabel = sym.fromtag("X")
dummyi = dummyj = None

whitespace = re.compile(r"\s+")
openbracket = re.compile(r"""(?:#(\d+))?\((\S+)""")
noderefre = re.compile(r"#([^)\s]+)")
labelre = re.compile(r"^(-?\d*)(?:<(\S+)>)?$")

def forest_lexer(s):
    si = 0
    while si < len(s):
        m = whitespace.match(s, si)
        if m:
            si = m.end()
            continue

        m = openbracket.match(s, si)
        if m:
            nodeid = m.group(1)
            label = m.group(2)

            if label == "OR":
                yield ('or', nodeid)
            else:
                m1 = labelre.match(label)
                if m1:
                    ruleid = m1.group(1)
                    vector = m1.group(2)
                    yield ('nonterm', nodeid, ruleid, vector)
                else:
                    raise TreeFormatException("couldn't understand label %s" % label)

            si = m.end()
            continue

        if s[si] == ')':
            si += 1
            yield ('pop',)
            continue

        m = noderefre.match(s, si)
        if m:
            noderef = m.group(1)
            yield ('ref', noderef)
            si = m.end()
            continue

        if s[si] == '"':
            sj = si + 1
            nodelabel = []
            while s[sj] != '"':
                if s[sj] == '\\':
                    sj += 1
                nodelabel.append(s[sj])
                sj += 1
            nodelabel = "".join(nodelabel)
            yield ('term', nodelabel)
            si = sj + 1
            continue

def forest_from_text(s, delete_words=[]):
    tokiter = forest_lexer(s)
    root = forest_from_text_helper(tokiter, {}, want_item=True, delete_words=delete_words).next()
    # check that all tokens were consumed
    try:
        tok = tokiter.next()
    except StopIteration:
        return root
    else:
        raise TreeFormatException("extra material after tree: %s" % (tok,))

def forest_from_text_helper(tokiter, memo, want_item=False, delete_words=[]):
    """Currently this assumes that the only frontier nodes in the tree are words."""
    while True:
        try:
            tok = tokiter.next()
            toktype = tok[0]
        except StopIteration:
            raise TreeFormatException("incomplete tree")

        if toktype == "or":
            _, nodeid = tok
            deds = list(forest_from_text_helper(tokiter, memo, delete_words=delete_words))
            node = Item(dummylabel, dummyi, dummyj, deds=deds)
            if nodeid:
                memo[nodeid] = node
            yield node

        elif toktype == "nonterm":
            _, nodeid, ruleid, dcoststr = tok
            if ruleid == "":
                ruleid = dummylabel
            else:
                ruleid = sym.fromtag(ruleid)
            dcost = svector.Vector()
            if dcoststr:
                for fv in dcoststr.split(','):
                    f,v = fv.split(':',1)
                    v = float(v)
                    dcost[f] = v

            ants = []
            rhs = []
            vi = 1
            for child in forest_from_text_helper(tokiter, memo, want_item=True, delete_words=delete_words):
                if isinstance(child, Item):
                    ants.append(child)
                    rhs.append(sym.setindex(dummylabel, vi))
                    vi += 1
                else:
                    rhs.append(child)
            r = rule.Rule(ruleid, rule.Phrase(rhs), rule.Phrase(rhs))

            node = Deduction(ants=ants, rule=r, dcost=dcost)
            if want_item: # need to insert OR node
                node = Item(dummylabel, dummyi, dummyj, deds=[node])
            if nodeid:
                memo[nodeid] = node
            yield node

        elif toktype == 'term':
            terminal = tok[1]
            if terminal not in delete_words:
                yield sym.fromstring(terminal)

        elif toktype == 'ref':
            yield memo[tok[1]]

        elif toktype == 'pop':
            return

        else:
            raise TreeFormatException("unknown token %s" % (tok,))

### Writing forests in XML
def forest_to_xml(node, fwords=None, mode=None, models=None, weights=None):
    result = []
    result.append('<forest>')
    if fwords:
        fwords = [(sym.tostring(fword) if type(fword) is int else fword) for fword in fwords]
        result.append('<source>%s</source>' % " ".join(fwords))

    _item_to_xml(node, result, {}, mode=mode, models=models, weights=weights)
    result.append('</forest>')
    return "".join(result)

def _item_to_xml(node, result, memo, mode, models, weights):
    if id(node) in memo:
        result.append(memo[id(node)])
        return

    nodeid = str(len(memo)+1)
    memo[id(node)] = "<or ref=%s/>" % xml.sax.saxutils.quoteattr(nodeid)

    states = []
    for mi,m in enumerate(models):
        try:
            s = m.strstate(node.states[mi])
        except IndexError:
            continue
        if s:
            states.append(s)
    states = ",".join(states)

    result.append('<or id=%s label=%s states=%s fspan=%s>' % (
            xml.sax.saxutils.quoteattr(nodeid),
            xml.sax.saxutils.quoteattr(sym.totag(node.x) if node.x else "None"),
            xml.sax.saxutils.quoteattr(states),
            xml.sax.saxutils.quoteattr("%s,%s" % (node.i,node.j))))

    # keep only the top k deductions to slim the forest
    deds = [(ded.viterbi, ded) for ded in node.deds]
    deds.sort()
    if max_deds:
        deds = deds[:max_deds]
    for _, ded in deds:
        _ded_to_xml(ded, result, memo, mode=mode, models=models, weights=weights)
    result.append('</or>')

def _ded_to_xml(node, result, memo, mode, models, weights):
    if weights:
        result.append('<and label=%s cost=%s>' % (xml.sax.saxutils.quoteattr(str(id(node.rule))),
                                                  xml.sax.saxutils.quoteattr(str(weights.dot(node.dcost)))))
    else:
        result.append('<and label=%s>' % (xml.sax.saxutils.quoteattr(str(id(node)))))

    result.append('<features>')
    for f,v in node.dcost.iteritems():
        result.append('<feature name=%s value=%s/>' % (xml.sax.saxutils.quoteattr(f), xml.sax.saxutils.quoteattr(str(v))))
    result.append('</features>')

    if mode == 'french':
        children = node.rule.f if node.rule else node.ants
    elif mode == 'english':
        children = node.rule.e if node.rule else node.ants
    else:
        children = node.ants

    for child in children:
        if isinstance(child, Item):
            _item_to_xml(child, result, memo, mode=mode, models=models, weights=weights)
        elif sym.isvar(child):
            _item_to_xml(node.ants[sym.getindex(child)-1], result, memo, mode=mode, models=models, weights=weights)
        else:
            result.append('<leaf label=%s/>' % xml.sax.saxutils.quoteattr(sym.tostring(child)))
    result.append('</and>')

### Writing forests in JSON
def forest_to_json(root, fwords=None, mode=None, models=None, weights=None):
    result = []
    result.append('{\n')

    if fwords:
        fwords = [(sym.tostring(fword) if type(fword) is int else fword) for fword in fwords]
        result.append('  "source": [%s],\n' % ",".join(quotejson(fword) for fword in fwords))

    items = list(root)
    nodeindex = {}
    nodestrs = []
    for ni,item in enumerate(items):
        nodeindex[item] = ni
        if item is root:
            ri = ni
        if item.x is None:
            nodestrs.append('    {}')
        else:
            nodestrs.append('    {"label": %s}' % quotejson(sym.totag(item.x)))
    result.append('  "nodes": [\n%s\n  ],\n' % ",\n".join(nodestrs))

    result.append('  "root": %d,\n' % ri)

    edgestrs = []
    for ni,item in enumerate(items):
        for ded in item.deds:
            tailstrs = []

            if mode == 'french':
                children = ded.rule.f if ded.rule else ded.ants
            elif mode == 'english':
                children = ded.rule.e if ded.rule else ded.ants
            else:
                children = ded.ants

            for child in children:
                if isinstance(child, Item):
                    tailstrs.append(str(nodeindex[child]))
                elif sym.isvar(child):
                    ant = ded.ants[sym.getindex(child)-1]
                    tailstrs.append(str(nodeindex[ant]))
                else:
                    tailstrs.append(quotejson(sym.tostring(child)))

            dcoststr = "{%s}" % ",".join("%s:%s" % (quotejson(f),v) for (f,v) in ded.dcost.iteritems())
            edgestrs.append('    {"head": %s, "tails": [%s], "features": %s}\n' % (
                    ni,
                    ",".join(tailstrs),
                    dcoststr))

    result.append('  "edges": [\n%s\n  ]\n' % ",\n".join(edgestrs))

    result.append('}')
    return "".join(result)

if __name__ == "__main__":
    import monitor
    import getopt

    weights = None
    opts, args = getopt.getopt(sys.argv[1:], "w:")
    for opt, optarg in opts:
        if opt == "-w":
            weights = svector.Vector(open(optarg).read())

    sys.stderr.write("t=%s start\n" % monitor.cpu())
    for li, line in enumerate(sys.stdin):
        f = forest_from_text(line)
        if weights:
            f.reweight(weights)
        print forest_to_xml(f, mode="english", weights=weights)
        sys.stderr.write("t=%s read line %s\n" % (monitor.cpu(), li))
        sys.stderr.flush()
