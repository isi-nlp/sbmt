#!/bin/env python
import tree
import sys
import re

d={"VP(NP-C PP)":1, "PP-BAR(NP-C IN)":1, "PP(NP-C IN)":1, "CONJP(RB IN)":1, "PP(VBN NP-C)":1}

relabeler1 = re.compile('-\d+$')
relabeler2 = re.compile('-\d+-BAR$')
relabeler3 = re.compile('^@')

def relabel(label):
    #sys.stderr.write("%sA\n" % label)
    label=relabeler2.sub('-BAR', label)
    label=relabeler1.sub('', label)
    label=relabeler3.sub('', label)
    return label



if __name__ == "__main__":
    for l in sys.stdin:
        (rid, trline) = l.split(' ', 1)
        node=tree.str_to_tree(trline)
        #sys.stdout.write("%s\n" % node.__str__())
        begin=0
        n_bad_rewrites=0
        for n in node.preorder():
            if(len(n.children)):
                #if begin!=0:
                #    sys.stdout.write(" ### ")
                begin=1
                rewrite="%s(" % relabel(n.label)
                ch=[]
                for c in n.children:
                    ch.append(relabel(c.label))
                #sys.stdout.write("%s" % ' '.join(ch))
                rewrite+= "%s" % ' '.join(ch)
                rewrite+=")"
                #sys.stdout.write("%s\n" % rewrite)
                if d.has_key(rewrite):
                    n_bad_rewrites=n_bad_rewrites+1
        sys.stdout.write("%s\tn_bad_rewrites=%s\n" % (rid, n_bad_rewrites))



