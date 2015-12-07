"""
Input:
  rule \t scores
Output:
  key \t rule \t scores
"""

import sys
import simplerule

def sig(frhs):
    mingap = 0
    for f in frhs:
        if isinstance(f, simplerule.Nonterminal):
            mingap += 1
        else:
            if mingap > 0:
                yield mingap
            mingap = 0
            yield '"%s"' % f
    if mingap > 0:
        yield mingap

for line in sys.stdin:
    line = line.rstrip()
    rule, _ = simplerule.Rule.from_str_hiero(line)
    print "%s\t%s" % (" ".join(str(f) for f in sig(rule.frhs)), line)
