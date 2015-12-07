#!/usr/bin/env python

# decoder.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

"""
There are three types of items:
  Item
  Rule
  DotItem
which go into three types of bins:
  Bin
  RuleBin
  list of DotItem
which can be subsetted to form lists:
  PseudoBin (subset by nonterminal symbol)
  RuleBin (subset by validate)
  n/a
which are fed into expand_cubeprune.

Additionally, each Item can have an n-best list.

"""

### To do:

# optimizations:
#   - speed up LM somehow?
#   - estimate_rule should only calculate cost for pruning
#     then we can go back and calculate statelesscost
#   - we should not build feature vectors online. do it offline

# remove most functions from Chart
# merge NewGrammar and DotChart?
# merge unary production handling???

# fix top-level functions
# process---translate---input, prepare_input
#                     +-Chart
#                     +-Chart.seed
#                     +-Chart.expand

# merge common code in n-best and cube pruning

import sys, gc, math, random
import os, os.path, gzip
import heapq, bisect, itertools, collections
import optparse, xml.sax.saxutils

import forest, model, rule, lattice, svector, cost
import log, monitor

IMPOSSIBLE = 999999
EPSILON = 0.000001

# how many derivations to keep
n_best = 1
# how many random derivations to append
n_random = 0
# how many ambiguous derivations (i.e., same English, different vector) to keep
ambiguity_limit = 5

def prepare_lattice(lat):
    # create index by j
    lat.edge_index = collections.defaultdict(list)
    for edge in lat.edges:
        lat.edge_index[edge.j].append(edge)

    # cache all-pairs shortest paths
    lat.distance = lat.compute_distance()

    return lat

def read_raw(infile):
    for li,line in enumerate(infile):
        lat = lattice.Lattice.from_words(line, id=li)
        yield lat

def read_lattices(infile):
    for line in infile:
        lat = lattice.Lattice.from_json(line)
        yield lat

class Decoder(object):
    def __init__(self, grammars, start_nonterminal, default_nonterminals, models, weights):
        self.set_grammar(grammars)
        self.start_nonterminal = start_nonterminal
        self.default_nonterminals = default_nonterminals
        self.models = models
        self.weights = weights

        # pruning parameters
        self.prune_threshold = None
        self.prune_limit = 100
        self.pop_limit = 1000

    def prepare_input(self, input):
        """hook that is called before translating each sentence"""
        pass

    def process_output(self, sent, outforest):
        """hook that is called after translating each sentence"""
        pass

    def set_grammar(self, gs, nonterminal_order=None):
        self.grammars = gs
        self.nonterminals = NonterminalAlphabet()
        if nonterminal_order is None:
            for g in self.grammars:
                if isinstance(g, Grammar):
                    for (x,y) in g.unary_less_than:
                        self.nonterminals.make_less_than(x,y)
                    self.nonterminals.update(g.nonterminals)
            self.nonterminals.topological_sort()
        else:
            self.nonterminals.nonterminals = nonterminal_order
            self.nonterminals.make_index()

    def translate(self, input):
        """input: a Lattice
           output: a forest"""

        if log.level >= 1:
            #log.write("Input: %s\n" % input) # problems with encoding
            pass

        if input.n == 0:
            return None

        input = prepare_lattice(input)

        # give each model a chance to prepare for input
        for m in self.models:
            m.input(input)

        self.prepare_input(input)

        chart = Chart(input, self.nonterminals, self.start_nonterminal, self.default_nonterminals, prune_threshold=self.prune_threshold, prune_limit=self.prune_limit)

        # transfer pruning parameters into chart
        chart.pop_limit = self.pop_limit

        chart.seed(input, self.grammars, self.models, self.weights)

        if log.level == 1:
            log.write("  Translating...\n")

        goal = chart.expand()

        if log.level >= 1:
            log.write("  items: %d added, %d merged, %d pruned, %d prepruned, %d discarded\n" % (chart.added, chart.merged, chart.pruned, chart.prepruned, chart.discarded))
            log.write("  dotitems: %d added\n" % (chart.dot_added))

        if log.level >= 2:
            chart.dump()

        return goal

##### Grammars

sentgrammar_file = None

class NonterminalAlphabet(object):
    def __init__(self):
        self.unary_children = {}
        self.nonterminals = set()

    def update(self, nonterminals):
        self.nonterminals.update(nonterminals)

    def make_less_than(self, x, y):
        self.unary_children.setdefault(y, set()).add(x)

    def topological_sort(self):
        """replace the set self.nonterminals with a list such that
           if there is a rule X -> Y, then Y precedes X in the list"""
        if log.level >= 3:
            log.write("Doing topological sort on nonterminals\n")
            if len(self.nonterminals) < 1000:
                log.write("  unsorted: %s\n" % " ".join(x.cat for x in self.nonterminals))
        nonterminals = []

        # make unary_children into graph
        for (x,s) in self.unary_children.items():
            for y in s:
                self.unary_children.setdefault(y, set())

        if log.level >= 3:
            log.writeln("  unary children:")
            for (x,s) in self.unary_children.items():
                log.write("    %s -> %s\n" % (x.cat, " | ".join(y.cat for y in s)))

        for x in self.nonterminals:
            if x not in self.unary_children:
                nonterminals.append(x)

        while len(self.unary_children) > 0:
            childless = None
            for (x,s) in self.unary_children.iteritems():
                if len(s) == 0:
                    childless = x
                    break
            if childless is None:
                childless = self.unary_children.keys()[0] # arbitrary
                sys.stderr.write("  warning: cycle detected, breaking all unary children of %s\n" % childless)
            del self.unary_children[childless]
            for (x,s) in self.unary_children.iteritems():
                s.discard(childless)
            nonterminals.append(childless)

        self.nonterminals = nonterminals
        if len(self.nonterminals) < 1000 and log.level >= 3:
            log.write("  sorted: %s\n" % " ".join(x.cat for x in self.nonterminals))

        self.make_index()

        self.unary_children = None

    def make_index(self):
        self.nonterminal_index = {}
        for i in xrange(len(self.nonterminals)):
            self.nonterminal_index[self.nonterminals[i]] = i

    def getrank(self, x):
        return self.nonterminal_index.get(x,None)

    def topological(self):
        return self.nonterminals

class Grammar(object):
    """
    Each trie node is a list [rulebin, next, vars]
      rulebin is a RuleBin containing the rules matched at this point
      next is a subtrie
      vars contains the possible left-hand sides
    """
    def __init__(self, threshold=None, limit=None):
        self.root = [None, {}]
        self.count = 0
        self.nonterminals = set()
        self.unary_rules = {}
        self.rulebin_count = 0
        self.unary_less_than = set()
        self.threshold = threshold
        self.limit = limit

    def add(self, r, estcost=0.):
        self.nonterminals.add(r.lhs)
        for f in r.frhs:
            if isinstance(f, rule.Nonterminal):
                self.nonterminals.add(f.clearindex())

        if r.arity() == 1 and len(r.frhs) == 1:
            log.write("unary rule: %s\n" % r)
            f = r.frhs[0].clearindex()
            self.unary_rules.setdefault(f, RuleBin(self.threshold, self.limit)).add(estcost, r)
            self.unary_less_than.add((f, r.lhs))
        else:
            cur = self.root
            for f in r.frhs:
                if isinstance(f, rule.Nonterminal):
                    f = f.clearindex()
                cur[1].setdefault(f, [None, {}])
                cur = cur[1][f]
            if cur[0] is None:
                cur[0] = RuleBin(self.threshold, self.limit)
                self.rulebin_count += 1
            bin = cur[0]
            bin.add(estcost, r)
            bin.prune()
        self.count += 1

    def read(self, f, models, weights):
        if type(f) is str:
            if os.path.isfile(f):
                if log.level >= 1:
                    log.write("Reading grammar from %s...\n" % f)
                f = file(f, 'r', 4*1024*1024)
            elif os.path.isfile("%s.gz" % f):
                f = "%s.gz" % f
                if log.level >= 1:
                    log.write("Decompressing grammar from %s...\n" % f)
                f = file(f, 'r', 4*1024*1024)
                f = gzip.GzipFile(fileobj=f)
        else:
            if log.level >= 1:
                log.write("Reading grammar...\n")

        for line in f:
            try:
                r = rule.Rule.from_str(line)
            except Exception:
                log.write("warning: couldn't scan rule %s\n" % line.strip())
                continue
            estcost = estimate_rule(r, models, weights)
            self.add(r, estcost)
            # self.add(rule.rule_from_line(line)) # this once caused a segfault
        log.write("%d rules read\n" % self.count)

    def _iter_helper(self, node):
        rbin, next = node
        if rbin:
            for _, r in rbin:
                yield r
        for _, child in next.iteritems():
            for r in self._iter_helper(child):
                yield r

    def __iter__(self):
        for _, rbin in self.unary_rules:
            for _, r in rbin:
                yield r
        for r in self._iter_helper(self.root):
            yield r

    def filterspan(self, i, j, n):
        return True

def estimate_rule(r, models, weights):
    '''Puts a lower-bound estimate inside the rule, returns
    the full estimate.'''

    r.statelesscost = svector.Vector()
    estcost = 0.

    for m in models:
        me = m.estimate(r)
        if m.stateless:
            r.statelesscost += me
        else:
            estcost += weights.dot(me)
    estcost += weights.dot(r.statelesscost)

    return estcost

class RuleBin(object):
    __slots__ = ['rules', 'f', 'lhs', 'cutoff', 'sorted', 'threshold', 'limit']
    pruned = 0

    '''A container for Rules that all have the same French side.'''
    def __init__(self, threshold, limit):
        object.__init__(self)

        self.rules = []
        heapq.heapify(self.rules)

        self.f = self.lhs = None

        self.cutoff = IMPOSSIBLE
        self.sorted = False

        self.threshold = threshold
        self.limit = limit

    def prune(self):
        while self.limit is not None and len(self.rules) > self.limit or -self.rules[0][0] >= self.cutoff:
            (negcost, r) = heapq.heappop(self.rules)

        if self.limit is not None and len(self.rules) == self.limit:
            self.cutoff = min(self.cutoff, -self.rules[0][0]+EPSILON)

    def add(self, cost, r, prune=True):
        if self.sorted: # expensive!
            self.rules = [(-cost,r) for (cost,r) in self.rules]
            heapq.heapify(self.rules)

        if prune and cost > self.cutoff:
            return

        if self.f is None:
            self.f = r.frhs
        else:
            r.frhs = self.f # share the f's in memory.
        if self.lhs is None:
            self.lhs = r.lhs
        heapq.heappush(self.rules, (-cost,r))

        if self.threshold is not None and cost+self.threshold < self.cutoff:
            self.cutoff = cost+self.threshold

        if prune:
            self.prune()

        self.sorted = False

    def __len__(self):
        return len(self.rules)

    def _ensure_sorted(self):
        if not self.sorted:
            newrules = [None] * len(self.rules)
            i = len(self.rules)-1
            while len(self.rules) > 0:
                (negcost, r) = heapq.heappop(self.rules)
                newrules[i] = (-negcost, r)
                i -= 1

            self.rules = newrules
            self.sorted = True

    def __getitem__(self, key):
        self._ensure_sorted()
        return self.rules[key]

    def __iter__(self):
        self._ensure_sorted()
        return iter(self.rules)

    def arity(self):
        # to do: make this nicer
        return self.rules[0][1].arity()

    def __str__(self):
        return "%s ::= %s (%d rules)" % (self.lhs, str(self.f), len(self.rules))

class DotItem(object):
    __slots__ = 'f', 'i', 'j', 'antbins', 'v'
    def __init__(self, f, i, j, antbins, v={}):
        self.f = f # pointer into Grammar trie
        self.i = i
        self.j = j
        self.antbins = antbins
        self.v = svector.Vector(v)

class DotChart(object):
    """One of these is created for each Grammar"""
    def __init__(self, chart, flattice):
        self.bins = [[[] for j in xrange(flattice.n)] for i in xrange(flattice.n)]
        self.chart = chart
        self.flattice = flattice

    def add(self, f, i, j, antbins, v={}):
        item = DotItem(f, i, j, antbins, v)
        if log.level >= 3:
            #log.write("Adding dotitem %d: %s,%d,%d,%s\n" % (id(item), f[0], i, j, ",".join(str(antbin) for antbin in antbins)))
            log.write("Adding dotitem %d: %s,%d,%d\n" % (id(item), f[0], i, j))
        self.chart.dot_added += 1
        self.bins[i][j].append(item)

    def expand_cell(self, i, j):
        """Create all DotItems for span (i,j) from smaller spans"""
        for k in xrange(i+1,j):
            self.extend_dotitems(i, k, j)
        # shift a terminal
        # can this be merged into extend_dotitems?
        for edge in self.flattice.edge_index[j]:
            for dotitem in self.bins[i][edge.i]:
                if edge.w in dotitem.f[1]:
                    self.add(dotitem.f[1][edge.w], i, j, dotitem.antbins, dotitem.v + edge.v)

    def expand_unary(self, i, j):
        self.extend_dotitems(i, i, j)

    def extend_dotitems(self, i, k, j):
        # shift a nonterminal
        vars = list(self.chart.bins[k][j].itervars())
        for dotitem in self.bins[i][k]:
            if log.level >= 3:
                log.write("Extending dotitem %d\n" % id(dotitem))
            for (y,ybin) in vars:
                if y in dotitem.f[1]:
                    self.add(dotitem.f[1][y], i, j, dotitem.antbins+(ybin,), dotitem.v)

    def get_cell(self, i, j):
        for dotitem in self.bins[i][j]:
            rulebin = dotitem.f[0]
            if rulebin is not None:
                yield (rulebin, dotitem.antbins, dotitem.v)

# merge this someday with DotChart
class NewGrammar(object):
    def __init__(self):
        pass

    def input(self, input):
        pass

    def add_edge(self, y, i, j, antbin):
        pass

    def get_rules(self, i, j):
        """returns a list of tuples (r, antbins)"""
        return []

class LatticeRules(NewGrammar):
    def input(self, lat):
        self.rules = collections.defaultdict(list)
        for span in lat.spans:
            i, j = span.i, span.j

            if hasattr(span, 'v'):
                v = svector.Vector(span.v)
            else:
                v = model.zero

            # bug: if new nonterminals are introduced at this point,
            # they will not participate in the topological sort

            r = rule.Rule(rule.Nonterminal(span.x),
                          [rule.Nonterminal.from_str(f) for f in span.f],
                          [rule.Nonterminal.from_str(e) for e in span.e],
                          scores=v)
            self.rules[i,j].append((r,))
            if log.level >= 2:
                log.write("added lattice rule at (%d,%d): %s\n" % (i,j,r))

    def get_rules(self, i, j):
        return self.rules[i, j]

##### Chart

class PseudoBin(object):
    def __init__(self, items=None):
        if items is None:
            self.items = []
        else:
            self.items = items

    def __getitem__(self, i):
        return self.items[i]

    def __len__(self):
        return len(self.items)

    def __iter__(self):
        return iter(self.items)

    def __str__(self):
        if len(self.items) > 0:
            #return "%s... (%d items)" % (self.items[0][1], len(self.items))
            return "PseudoBin(%s)" % ",".join(str(item) for (totalcost, item) in self.items)
        else:
            return "empty PseudoBin"

    def append(self, x):
        self.items.append(x)

class Bin(object):
    def __init__(self, chart, threshold=IMPOSSIBLE, limit=None):
        # elements are [-totalcost, item]
        # -totalcost because we want the queue to be worst-first
        self.queue = []
        self.index = {}
        self.dead = 0
        self.best = IMPOSSIBLE
        self.cutoff = IMPOSSIBLE
        self.threshold = threshold
        self.limit = limit
        self.chart = chart # just for accounting
        self.sorted = None
        self.varindex = {}

    def add(self, totalcost, item):
        result = False
        oldentry = self.index.get(item, None)
        if oldentry is not None:
            (oldtotalcost, olditem) = oldentry
            oldtotalcost = -oldtotalcost

            if log.level >= 3:
                log.write("Merging: %s using %s with %s, totalcost=%f\n" % (str(item), str(item.deds[0]), str(self.index[item][1]), totalcost))
            if self.chart is not None:
                self.chart.merged += 1

            if totalcost < oldtotalcost:
                # Kill the old item and merge into the new one
                oldentry[1] = None # kill in self.queue
                self.dead += 1
                if type(item) is forest.Item:
                    item.deds.extend(olditem.deds)

                # Add the new one
                entry = [-totalcost, item]
                heapq.heappush(self.queue, entry)
                self.index[item] = entry # displace old item
                self.best = min(self.best, totalcost)
                self.sorted = None
                result = True
            else:
                # Merge the new one into the old one
                if type(olditem) is forest.Item:
                    olditem.deds.extend(item.deds)
        else:
            if log.level >= 3:
                if type(item) is forest.Item:
                    log.write("Adding: %s using %s, totalcost=%s\n" % (str(item), str(item.deds[0]), totalcost))
                    log.write("  from %s\n" % (" ".join(str(ant) for ant in item.deds[0].ants)))
                else:
                    log.write("Adding: %s\n" % str(item))
            if self.chart is not None:
                self.chart.added += 1
            entry = [-totalcost, item]
            heapq.heappush(self.queue, entry)
            self.index[item] = entry
            self.best = min(self.best, totalcost)
            self.sorted = None
            result = True

        if self.threshold is not None:
            self.cutoff = min(self.best+self.threshold, IMPOSSIBLE)

        self.prune()

        if log.level >= 3:
            log.write(" bin best/worst is %s/%s\n" % (self.best, -self.queue[0][0]))
            log.write(" bin now has %d items\n" % (len(self)))
            log.write(" bin cutoff is now %s\n" % self.cutoff)

        return result

    def __len__(self):
        return len(self.queue)-self.dead

    def prune(self):
        # This method is more fastidious than Pharaoh's. Pharaoh only
        # seems to do threshold pruning if the prune_limit is
        # exceeded.
        if len(self.queue)-self.dead == 0:
            return

        while self.limit is not None and len(self.queue)-self.dead > self.limit or -self.queue[0][0] >= self.cutoff:
            (negtotalcost, item) = heapq.heappop(self.queue)
            if item is None:
                self.dead -= 1
            else:
                del self.index[item]
                if self.chart is not None:
                    self.chart.pruned += 1

        if self.limit is not None and len(self.queue)-self.dead == self.limit:
            self.cutoff = min(self.cutoff, -self.queue[0][0]+EPSILON)

    # These methods are only to be used after the cell is finished

    def _ensure_sorted(self):
        if self.sorted is None:
            self.sorted = [(-negtotalcost, item) for (negtotalcost, item) in self.index.values()]
            self.sorted.sort()
            self.varindex = {}
            for (cost, item) in self.sorted:
                self.varindex.setdefault(item.x, PseudoBin()).append((cost, item))

    def __getitem__(self, key):
        self._ensure_sorted()
        return self.sorted[key]

    def __iter__(self):
        self._ensure_sorted()
        return iter(self.sorted)

    def itervars(self):
        self._ensure_sorted()
        return self.varindex.iteritems()

class Cube(object):
    def __init__(self, rulebin, antbins, latticev):
        self.bins = [rulebin]+list(antbins)
        self.latticev = latticev

    def __len__(self):
        n = 1
        for bin in self.bins:
            n *= len(bin)
        return n

    def first(self):
        return (0,)*len(self.bins)

    def n_predecessors(self, ranks):
        n = 0
        for r in ranks:
            if r > 0:
                n += 1
        return n

    def successors(self, ranks):
        s = []
        for i in xrange(len(ranks)):
            if ranks[i]+1 < len(self.bins[i]):
                s.append(tuple(r+1 if j==i else r for j,r in enumerate(ranks)))
        return s
        
    def __getitem__(self, ranks):
        values = []
        for bin, rank in itertools.izip(self.bins, ranks):
            _, value = bin[rank] # discard left member, which was used for sorting only
            values.append(value)
        r = values[0]
        ants = tuple(values[1:])
        return (r, ants)

class Chart(object):
    def __init__(self, flattice, nonterminals, start_nonterminal, default_nonterminals, prune_threshold, prune_limit):
        object.__init__(self)

        self.added = self.merged = self.prepruned = self.pruned = self.discarded = 0
        self.max_popped = 0
        self.unary_pruned = 0
        self.dot_added = 0

        ### Create bins
        self.flattice = flattice
        n = flattice.n
        self.nonterminals = nonterminals
        self.start_nonterminal = start_nonterminal
        self.default_nonterminals = default_nonterminals
        self.bins = [[None for j in xrange(n)] for i in xrange(n)]
        for i in xrange(n):
            for j in xrange(n):
                self.bins[i][j] = Bin(self,
                                      prune_threshold,
                                      prune_limit)
        self.goal = Bin(self, IMPOSSIBLE, None)

        self.stateful_m_is = None

    def bin(self, item):
        if type(item) is forest.Item:
            return self.bins[item.i][item.j]
        else:
            raise ValueError

    def topological(self):
        for bin in self.topological_bins():
            for (totalcost,item) in bin:
                yield item

    def topological_bins(self):
        for l in xrange(1,self.flattice.n):
            for i in xrange(self.flattice.n-l):
                j = i+l
                for x in self.nonterminals.topological():
                    if x in self.bins[i][j].varindex:
                        yield self.bins[i][j].varindex[x]
        yield self.goal

    def dump(self):
        for i in xrange(self.flattice.n):
            for j in xrange(i,self.flattice.n):
                log.write("Span (%d,%d):\n" % (i,j))
                for (totalcost,item) in self.bins[i][j]:
                    log.write("%s totalcost=%f\n" % (str(item),totalcost))
        log.write("Goals:\n")
        for (totalcost,item) in self.goal:
            log.write("%s totalcost=%f\n" % (str(item),totalcost))

    def seed(self, flattice, grammars, models, weights):
        self.models = models
        self.weights = weights

        # Seed the dotchart. This will give the extracted rules

        self.grammars = [(g, DotChart(self, flattice)) for g in grammars if isinstance(g, Grammar)]

        for (g,dotchart) in self.grammars:
            for i in xrange(self.flattice.n-1):
                if g.filterspan(self.flattice, i, i):
                    dotchart.add(g.root,i,i,())
                    self.dot_added += 1

        for g in grammars:
            if isinstance(g, NewGrammar):
                g.input(flattice)
                for i in xrange(self.flattice.n-1):
                    for j in xrange(i+1,self.flattice.n):
                        for (r,) in g.get_rules(i,j):
                            estimate_rule(r, models, weights)
                            self.add_axiom(i, j, r)

        # Last resort for unknown French word: pass it through
        for edge in flattice.edges:
            for x in self.default_nonterminals:
                r = rule.Rule(x,
                              [edge.w],
                              [edge.w],
                              scores=svector.Vector('unknown', 1.))
                estimate_rule(r, models, weights)
                self.add_axiom(edge.i, edge.j, r)

    def add_axiom(self, i, j, r, latticev=model.zero):
        bin = self.bins[i][j]
        (totalcost, (cost, dcost, newstates)) = self.compute_item(r, (), i, j, latticev)
        if totalcost < bin.cutoff:
            ded = forest.Deduction((), r, dcost, viterbi=cost)
            item = forest.Item(r.lhs, i, j, deds=[ded], states=newstates, viterbi=cost)
            bin.add(totalcost, item)
        else:
            if log.level >= 4:
                log.write("Prepruning: %s\n" % r)
            self.prepruned += 1

    def compute_item(self, r, ants, i, j, latticev=model.zero):
        """Computes various pieces of information that go into an Item:
        heuristic (float), for comparing Items
        cost (float), of the resulting Item
        dcost (Vector), to be stored in Deduction
        states

        The reason this isn't just part of Item.__init__() is that
        we want to be able to abort creation of an Item object
        as early as possible.

        It didn't really need to be a method of Chart.
        """

        ms = self.models
        w = self.weights

        cost = sum(ant.viterbi for ant in ants)
        dcost = svector.Vector(r.statelesscost) + latticev
        bonus = svector.Vector()
        newstates = [None]*len(ms)

        if r.arity() == 2:
            j1 = ants[0].j
        else:
            j1 = None

        for m_i in xrange(len(ms)):
            m = ms[m_i]
            if not m.stateless:
                antstates = [ant.states[m_i] for ant in ants]
                (state, mdcost) = m.transition(r, antstates, i, j, j1)
                bonus += m.bonus(r.lhs, state)
                newstates[m_i] = state
                dcost += mdcost
        cost += w.dot(dcost)
        return (cost+w.dot(bonus), (cost, dcost, newstates))

    def expand_goal(self, bin1):
        for (cost1, item1) in bin1:
            if item1.x == self.start_nonterminal:
                if log.level >= 3:
                    log.write("Considering: %s\n" % str(item1))
                dcost = sum((m.finaltransition(item1.states[m_i]) for (m_i,m) in enumerate(self.models)), svector.Vector())
                cost = item1.viterbi+self.weights.dot(dcost)
                ded = forest.Deduction((item1,), None, dcost, viterbi=cost)
                self.goal.add(cost, forest.Item(None, 0, self.flattice.n-1, deds=[ded], states=(), viterbi=cost))

    def expand_cell_cubeprune(self, i, j, cubes):
        # initialize candidate list
        cand = []
        index = collections.defaultdict(int)
        for cube in cubes:
            if len(cube) > 0:
                ranks = cube.first()
                r, ants = cube[ranks]
                (totalcost, info) = self.compute_item(r, ants, i, j, cube.latticev)
                cand.append((totalcost, info, cube, ranks))
                index[cube, ranks] += 1
        heapq.heapify(cand)

        bin = self.bins[i][j]

        popped = 0
        while len(cand) > 0 and (self.pop_limit is None or popped < self.pop_limit):
            # Get the best item on the heap
            (totalcost, (cost, dcost, newstates), cube, ranks) = heapq.heappop(cand)
            popped += 1
            r, ants = cube[ranks]

            if totalcost < bin.cutoff:
                # Turn it into a real Item
                ded = forest.Deduction(ants, r, dcost, viterbi=cost)
                item = forest.Item(r.lhs, i, j, deds=[ded], states=newstates, viterbi=cost)
                bin.add(totalcost, item)
            else:
                self.prepruned += 1

            # Put item's successors into the heap
            for nextranks in cube.successors(ranks):
                index[cube, nextranks] += 1
                if index[cube, nextranks] == cube.n_predecessors(nextranks):
                    r, ants = cube[nextranks]
                    (totalcost, info) = self.compute_item(r, ants, i, j)
                    heapq.heappush(cand, (totalcost, info, cube, nextranks))

        self.discarded += len(cand)
        self.max_popped = max(self.max_popped, popped)

    def expand_unary(self, i, j):
        """Finish bin (i,j) by building items with unary productions."""
        agenda = [(self.nonterminals.getrank(item.x), totalcost, item) for (totalcost, item) in self.bins[i][j]]
        heapq.heapify(agenda)
        while len(agenda) > 0:
            (trank, _, titem) = heapq.heappop(agenda)
            if log.level >= 3:
                log.write("Applying unary rules to %s\n" % titem)

            # it may happen that the item was defeated or pruned before we got to it
            if titem not in self.bins[i][j].index:
                continue

            for (g,dotchart) in self.grammars:
                if g.filterspan(self.flattice, i, j):
                    for (estcost, r) in g.unary_rules.get(titem.x, ()):
                        rank = self.nonterminals.getrank(r.lhs)

                        # if the new item isn't of lower priority
                        # than the current trigger item (because of
                        # a unary cycle), adding it could corrupt
                        # the forest
                        if rank <= trank:
                            self.unary_pruned += 1
                            continue

                        (totalcost, (cost, dcost, newstates)) = self.compute_item(r, (titem,), i, j)
                        ded = forest.Deduction((titem,), r, dcost, viterbi=cost)
                        item = forest.Item(r.lhs, i, j, deds=[ded], states=newstates, viterbi=cost)
                        if self.bins[i][j].add(totalcost, item):
                            heapq.heappush(agenda, (rank, totalcost, item))

    def expand(self):
        n = self.flattice.n
        bins_done = 0
        for l in xrange(1,n):
            for i in xrange(n-l):
                j = i+l

                if log.level >= 2:
                    log.write("Filling dotcell (%d,%d)\n" % (i,j))

                for (g,dotchart) in self.grammars:
                    if g.filterspan(self.flattice, i, j):
                        dotchart.expand_cell(i,j)

                cubes = []
                for (g,dotchart) in self.grammars:
                    for rulebin, antbins, latticev in dotchart.get_cell(i, j):
                        if rulebin.arity() == 0:
                            for (cost,r) in rulebin:
                                self.add_axiom(i, j, r, latticev)
                        else:
                            cubes.append(Cube(rulebin, antbins, latticev))

                if log.level >= 2:
                    log.write("Filling cell (%d,%d)\n" % (i,j))
                self.expand_cell_cubeprune(i, j, cubes)

                # Process unary rules
                self.expand_unary(i,j)

                # Deal with zero-width dotitems
                for (g,dotchart) in self.grammars:
                    if g.filterspan(self.flattice, i, j):
                        dotchart.expand_unary(i,j)

                bins_done += 1
        self.expand_goal(self.bins[0][n-1])

        if len(self.goal) == 1:
            return self.goal[0][1]
        elif len(self.goal) > 1:
            log.write("Warning: multiple goal items\n")
            return self.goal[0][1]
        else:
            return None

def get_nbest(goal, n_best, ambiguity_limit=None, n_random=None):
    """Input: goal item
    Output: n-best list (unique + random)"""

    result = []

    nbest = forest.NBest(goal, ambiguity_limit=ambiguity_limit)
    for deriv in itertools.islice(nbest, n_best):
        result.append((deriv.vector(), deriv.english()))

    if n_random:
        insides = goal.compute_inside(weights) # global
        for i in xrange(n_random):
            deriv = goal.random_deriv(insides)
            result.append((deriv.vector(), deriv.english()))

    return result

def makefilename(basename):
    if opts.parallel:
        return "%s.part%04d" % (basename, parallel.rank)
    else:
        return basename

if __name__ == "__main__":
    try:
        gc.set_threshold(100000,10,10)
    except:
        pass

    # Set up command-line options

    optparser = optparse.OptionParser()
    optparser.add_option("-p", "--parallel", dest="parallel", help="parallelize using MPI", action="store_true")
    optparser.add_option("-m", dest="n_best_file")
    optparser.add_option("-f", dest="forest_dir", help="directory where forests should be output")
    optparser.add_option("--input-lattice", dest="input_lattice", action="store_true")
    optparser.add_option("--output-french-parses", dest="french_parse_file")
    optparser.add_option("--output-english-parses", dest="english_parse_file")
    optparser.add_option("--output-rule-posteriors", dest="rule_posterior_dir")

    # the config file's job is to bind a global variable
    # called thedecoder that belongs to a subclass of Decoder.

    # the config file is allowed to add options to optparser
    # and to call optparser.parse_args.

    try:
        configfilename = sys.argv[1]
    except IndexError:
        sys.stderr.write("usage: decoder.py config-file [input] [output] [options...]\n")
        sys.exit(1)

    if log.level >= 1:
        log.write("Reading configuration from %s\n" % configfilename)
    execfile(configfilename)

    opts, args = optparser.parse_args(args=sys.argv[2:])

    if opts.parallel:
        import parallel
        log.prefix="[%s] " % parallel.rank

    if not opts.parallel or parallel.rank == parallel.master:
        if len(args) >= 1 and args[0] != "-":
            input_file = file(args[0], "r")
        else:
            input_file = sys.stdin

        if len(args) >= 2 and args[1] != "-":
            output_file = file(args[1], "w")
        else:
            output_file = sys.stdout

        if opts.input_lattice:
            insents = read_lattices(input_file)
        else:
            insents = read_raw(input_file)
    else:
        insents = [] # dummy

    if opts.n_best_file:
        n_best_file = open(makefilename(opts.n_best_file), "w")
    else:
        n_best_file = None

    if not opts.parallel or parallel.rank == parallel.master:
        if opts.forest_dir:
            if not os.access(opts.forest_dir, os.F_OK):
                os.makedirs(opts.forest_dir)

        if opts.rule_posterior_dir:
            if not os.access(opts.rule_posterior_dir, os.F_OK):
                os.makedirs(opts.rule_posterior_dir)

    if opts.french_parse_file:
        french_parse_file = open(makefilename(opts.french_parse_file), "w")
    else:
        french_parse_file = None

    if opts.english_parse_file:
        english_parse_file = open(makefilename(opts.english_parse_file), "w")
    else:
        english_parse_file = None

    if not opts.parallel or parallel.rank != parallel.master:
        thedecoder = make_decoder()
        if log.level >= 1:
            gc.collect()
            log.write("all structures loaded, memory=%s\n" % (monitor.memory(),))

    def process(sent):
        goal = thedecoder.translate(sent)

        thedecoder.process_output(sent, goal)

        if goal is None:
            log.writeln("warning: parse failure")
            return None

        if opts.forest_dir:
            forest_file = gzip.open(os.path.join(opts.forest_dir, "forest.%s.gz" % sent.id), "w")
            forest_file.write(forest.forest_to_json(goal, fwords=sent.words, mode='english', models=thedecoder.models, weights=thedecoder.weights))
            forest_file.close()

        if opts.rule_posterior_dir:
            rule_posterior_file = open(os.path.join(opts.rule_posterior_dir, "rule_posterior.%s" % sent.id), "w")
            beta = 1.
            insides = goal.compute_inside(thedecoder.weights, beta=beta)
            outsides = goal.compute_outside(thedecoder.weights, insides, beta=beta)
            z = insides[id(goal)]
            for item in goal.bottomup():
                for ded in item.deds:
                    c = outsides[id(item)]
                    c += thedecoder.weights.dot(ded.dcost)
                    c += sum(insides[id(ant)] for ant in ded.ants)
                    c -= z
                    rule_posterior_file.write("%s ||| span=%s posterior=%s\n" % (ded.rule, (item.i, item.j), cost.prob(c)))
                    ded.dcost['posterior'] = c
            rule_posterior_file.close()
            max_posterior_file = open(os.path.join(opts.rule_posterior_dir, "max_posterior.%s" % sent.id), "w")
            goal.reweight(svector.Vector('posterior=1'))
            max_posterior = goal.viterbi_deriv()

            def show(ded, antvalues):
                if ded.rule:
                    value = rule.subst(ded.rule.erhs, antvalues)
                else:
                    value = antvalues[0]
                return ("[%.3f" % cost.prob(ded.dcost['posterior']),) + value + ("]",)
            value = max_posterior.value(show)
            s = " ".join(value)
            max_posterior_file.write("%s\n" % s)

            max_posterior_file.close()


        outputs = get_nbest(goal, n_best, ambiguity_limit)


        if n_best_file:
            for (v,e) in outputs:
                e = " ".join(e)
                #n_best_file.write("%s ||| %s ||| %s\n" % (sent.id, e, -thedecoder.weights.dot(v)))
                n_best_file.write("%s ||| %s ||| %s\n" % (sent.id, e, v))
            n_best_file.flush()

        (bestv,best) = outputs[0]

        if french_parse_file:
            french_parse_file.write("%s ||| %s\n" % (sent.id, goal.viterbi_deriv().french_tree()))
            french_parse_file.flush()
        if english_parse_file:
            english_parse_file.write("%s ||| %s\n" % (sent.id, goal.viterbi_deriv().english_tree()))
            english_parse_file.flush()

        if log.level >= 1:
            gc.collect()
            log.write("  done decoding, memory=%s\n" % monitor.memory())
            log.write("  features: %s; %s\n" % (bestv, thedecoder.weights.dot(bestv)))

        sent.ewords = best
        return sent

    if opts.parallel:
        outsents = parallel.pmap(process, insents, tag=0, verbose=1)
    else:
        outsents = (process(sent) for sent in insents)

    if not opts.parallel or parallel.rank == parallel.master:
        for outsent in outsents:
            if outsent is None:
                output_file.write("\n")
            else:
                output_file.write("%s\n" % " ".join(outsent.ewords))
            output_file.flush()
