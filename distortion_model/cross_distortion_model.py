import math, collections
import sys

class InfoFactory(object):
    def __init__(self, sc, g, pmap):
        self.feature_id = pmap["cross"]
        self.weight = sc["distortion"]
        
    def rule_heuristic(self, g, r):
        return 0.

    def scoreable_rule(self, g, r):
        return g.is_complete_rule(r)

    def compute(self, g, r, child_infos):
        """Compute length of new parent and P(cross|span) for each child"""
        if g.rule_has_property(r, self.feature_id):
            cross = g.rule_property(r, self.feature_id)
        else:
            cross = [] # make this None after file is fixed
        sr = g.get_syntax(r)
        #print_rule(g, sr)
        length = 0
        ci = 0
        cost = 0.
        for node in sr.rhs:
            if node.lexical():
                length += 1
            else:
                cl = child_infos[ci]
                length += cl
                if cross is not None:
                    try:
                        cc = cross[ci]
                    except IndexError: # workaround
                        cc = 0
                    cx = g.label(node.token)
                    cp = costs.get((cl,cx,cc), None)
                    if cp is None:
                        cp = costs[None,None,cc]
                    #print "child length = %d, label = %s, cross = %d => %s" % (cl, cx, cc, cp)
                    cost += cp
                ci += 1
        return (length, cost)

    def create_info(self, g, r, child_infos):
        length, cost = self.compute(g, r, child_infos)
        yield (length, cost*self.weight, 0.) # note: yield, not return
        
    def component_scores(self, g, r, child_infos, result):
        length, cost = self.compute(g, r, child_infos)
        return [cost]

    def component_score_names(self):
        return ["distortion"]

import re
delim = re.compile(r"""[ ,] *""")
def read_cross(s):
    # workaround
    s = s.strip()
    if s == "":
        return []
    else:
        try:
            return [int(c) for c in delim.split(s) if c != "10^-"]
        except:
            sys.stderr.write("couldn't scan cross feature: %s\n" % s)

counts = collections.defaultdict(lambda: collections.defaultdict(int))
for line in file("counts.label"):
    span, label, cross, count = line.split()
    counts[int(span),label][int(cross)] = int(count)

# default
counts[None,None][0] = 0
counts[None,None][1] = 0

costs = {}
for span,label in counts.iterkeys():
    counts[span,label][0] += 9
    counts[span,label][1] += 1
    denom = float(counts[span,label][0]+counts[span,label][1])
    costs[span,label,0] = -math.log10(counts[span,label][0]/denom)
    costs[span,label,1] = -math.log10(counts[span,label][1]/denom)

register_info_factory_constructor("distortion2", InfoFactory)
register_rule_property_constructor("distortion2", "cross", read_cross)
