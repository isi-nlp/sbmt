"""
Count French words in rules. Used by xrsdb.

Input:
  lhs ||| frhs ||| erhs
Output:
  f \t 1
  f \t 1
  ...
"""

import sys
import simplerule

for line in sys.stdin:
    line = line.rstrip()
    rule, _ = simplerule.Rule.from_str_hiero(line)
    for f in rule.frhs:
        if not isinstance(f, simplerule.Nonterminal):
            print "%s\t%s" % (f,1)
            
