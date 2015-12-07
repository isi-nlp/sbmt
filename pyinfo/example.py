import math
import sys

# Demonstrate how to recover all information about an xRS rule

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

def print_brf(g, rule):
    sys.stdout.write("%s -> %s\n" % (g.label(g.rule_root(rule)), " ".join(g.label(x) for x in g.rule_rhs(rule))))

class InfoFactory(object):
    def __init__(self, sc, g, pmap):
        self.feature_id = pmap["cross"]
        self.weight = sc["distortion"]
        
    def rule_heuristic(self, g, r):
        print_brf(g, r)
        
        """if g.is_complete_rule(r):
            sr = g.get_syntax(r)
            print_rule(g, sr)"""
            
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
                        c = cross[ci]
                    except IndexError: # workaround
                        c = 0
                    #print "child length = %d, cross = %d => %s" % (cl, c, costs[c,cl])
                    cost += costs[c,cl]
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

def read_cross(s):
    cross = []
    for c in s.split(','):
        if c.startswith("10^-"):
            cross.append(int(c[4:])) # workaround
        elif c.strip() == "":
            pass
        else:
            cross.append(int(c))
    return cross
    #return [int(c) for c in s.split() if c != "10^-"] # workaround

costs = {}
for line in file("probs"):
    cross, span, prob = line.split()
    costs[int(cross),int(span)] = -math.log10(float(prob))

register_info_factory_constructor("distortion2", InfoFactory)
register_rule_property_constructor("distortion2", "cross", read_cross)
