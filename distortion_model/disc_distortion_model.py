import math, collections
import sys
import pysbmt

features = ["dist[%s;%s]" % (cross, size) for cross in [False, True] for size in [1,2,3,4,5,6,7,8,9,"10+","*"]]

class InfoFactory(object):
    def __init__(self, sc, g, pmap):
        self.cross_id = pmap["cross"]
        self.weights = dict((feat, sc[feat]) for feat in features)
        
    def rule_heuristic(self, g, r):
        return 0.

    def scoreable_rule(self, g, r):
        #return g.is_complete_rule(r)
        return True

    def compute(self, g, r, child_infos):
        """Compute length of new parent and P(cross|span) for each child"""
        if g.rule_has_property(r, self.cross_id):
            cross = g.rule_property(r, self.cross_id)
        else:
            cross = None

        if pysbmt.is_lexical(g.rule_root(r)): # ignore dummy rules with terminal lhs
            return (0, collections.defaultdict(int))
            
        length = 0
        child_i = cross_i = 0
        v = collections.defaultdict(int)
        for f in g.rule_rhs(r):
            if pysbmt.is_lexical(f):
                length += 1
            else:
                cl = child_infos[child_i]
                length += cl
                if not pysbmt.is_virtual_tag(f) and cross is not None:
                    try:
                        cc = cross[cross_i]
                    except IndexError:
                        sys.stderr.write("IndexError: cross = %s, ci = %s" % (cross,cross_i))
                        continue
                            
                    cx = g.label(f)
                    v["dist[%s;%s]" % (cc,cl if cl < 10 else "10+")] += 1
                    v["dist[%s;*]" % (cc,)] += 1
                    cross_i += 1
                child_i += 1
        return (length, v)

    def create_info(self, g, r, child_infos):
        length, v = self.compute(g, r, child_infos)
        cost = 0.
        for (f,c) in v.iteritems():
            cost += self.weights[f]*c
        yield (length, cost, 0.) # note: yield, not return
        
    def component_scores(self, g, r, child_infos, result):
        length, v = self.compute(g, r, child_infos)
        return [v[f] for f in features]

    def component_score_names(self):
        return list(features)

def read_cross(s):
    return [bool(int(c)) for c in s.split()]

register_info_factory_constructor("distortion3", InfoFactory)
register_rule_property_constructor("distortion3", "cross", read_cross)

def print_tree(g, node, rhs):
    if node.is_leaf():
        if node.lexical():
            sys.stdout.write(g.label(node.token))
        else:
            sys.stdout.write("%s:%s" % (rhs[node.rhs_position], g.label(node.token)))
    else:
        sys.stdout.write("%s(" % g.label(node.token))
        for child in node.children():
            sys.stdout.write(" ")
            print_tree(g, child, rhs)
            sys.stdout.write(")")


def print_rule(g, rule):
    rhs = []
    vi = 0
    for node in rule.rhs:
        if node.lexical():
            rhs.append(g.label(node.token))
        else:
            rhs.append("x%d" % vi)
            vi += 1
        
    print_tree(g, rule.lhs_root, rhs)
    sys.stdout.write(" -> ")
    sys.stdout.write(" ".join(rhs))
    sys.stdout.write("\n")

