#!/usr/bin/env python

import re
import sys

rule_expr = re.compile("([^\\(]+)\\(.*x0:([^\\)\\s]+).*\\) -> x0 ###")
textual_expr = re.compile(".* text-length=")

edges = []
ids = {}
id = 0


for rule in sys.stdin:
    m = rule_expr.match(rule)
    textual = False
    if m:
        for v in m.group(1,2):
            if v not in ids:
                ids[v] = id
                id += 1
            if textual_expr.match(rule):
                textual = True
        edges.append((m.group(2),m.group(1),textual))

print "digraph {"
#for node in ids.keys():
#    print "node [ label = \"%s\" ] _%s ;" % (node,ids[node])
for line in edges:
    label="black"
    if line[2]:
        label="blue"
    print "\"%s\" -> \"%s\" [color=\"%s\"];" % (line[0],line[1],label)
print "}"
