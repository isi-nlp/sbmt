"""
Convert Hadoop-style rules to Hiero-style rules.

Input:
  [LHS] ||| frhs ||| erhs ||| align={{{f-e f-e f-e}}} \t feat=value feat=value

Output:
  [LHS] ||| frhs ||| erhs ||| feat=value feat=value ||| align={{{f-e f-e f-e}}}
"""

import sys
import svector
import simplerule

for line in sys.stdin:
    try:
        rule, scores = line.rstrip().split('\t')
        rule = simplerule.Rule.from_str(rule)
    except Exception:
        sys.stderr.write("bad line: %s\n" % line.rstrip())
    print rule.str_hiero3(scores)
