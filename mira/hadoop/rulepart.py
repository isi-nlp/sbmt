"""
Input:
  rule \t scores
Output:
  key \t rule \t scores
"""

import sys
import simplerule
try:
    import tree
except ImportError:
    pass

part = sys.argv[1]

def normalize(x):
    if isinstance(x, simplerule.Nonterminal):
        return str(x.clearindex())
    else:
        return str(x)

def normalize_tree(t):
    t = tree.Node.from_str(t)
    for leaf in t.frontier():
        leaf.label = normalize(simplerule.Nonterminal.from_str(leaf.label))
    return str(t)

for line in sys.stdin:
    line = line.rstrip()
    rule, _ = line.split('\t', 1)
    rule = simplerule.Rule.from_str(rule)
    if part == "frhs":
        key = " ".join(normalize(f) for f in rule.frhs)
    elif part == "erhs":
        key = " ".join(normalize(e) for e in rule.erhs)

    elif part == "rule":
        rule.attrs = simplerule.Attributes()
        key = str(rule)
    
    elif part == "ftree":
        key = normalize_tree(rule.attrs['ftree'])
    elif part == "etree":
        key = normalize_tree(rule.attrs['etree'])
    elif part == "fetree":
        key = "%s ||| %s" % (normalize_tree(rule.attrs['ftree']), normalize_tree(rule.attrs['etree']))
    else:
        raise ValueError("unknown field %s" % key)

    print "%s\t%s" % (key, line)
