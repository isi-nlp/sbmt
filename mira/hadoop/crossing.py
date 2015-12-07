#!/usr/bin/env python

import sys, collections
import simplerule, svector

barriers = "( ) ' \" . ? ! , : ; -".split()

class SkipRule(Exception):
    pass

for line in sys.stdin:
    try:
        r, scores = line.rstrip().split('\t', 1)
        scores = svector.Vector(scores)
        r = simplerule.Rule.from_str(r)
        a = [tuple(int(i) for i in s.split('-',1)) for s in r.attrs['align'].split()]

        # add nonterminals to alignment
        for (fj,f) in enumerate(r.frhs):
            if isinstance(f, simplerule.Nonterminal):
                for (ej,e) in enumerate(r.erhs):
                    if isinstance(e, simplerule.Nonterminal) and f.getindex() == e.getindex():
                        a.append((fj,ej))

        ai = collections.defaultdict(list)
        for (fi,ei) in a:
            ai[fi].append(ei)

        for (fi,f) in enumerate(r.frhs):
            if isinstance(f, simplerule.Nonterminal):
                if len(ai[fi]) > 1:
                    sys.stderr.write("warning: malformed alignments: %s\n" % r)
                    raise SkipRule
                #assert len(ai[fi]) == 1
                ei = ai[fi][0]
                for fj,ej in a:
                    if fj < fi and ej > ei or fj > fi and ej < ei:
                        # this really should be put into attrs
                        try:
                            scores['cross%s' % f.getindex()] = 1.
                        except TypeError:
                            sys.stderr.write("warning: malformed rule: %s\n" % r)
                            raise SkipRule
            elif f in barriers:
                for ei in ai[fi]:
                    if r.erhs[ei] == f:
                        for fj,ej in a:
                            if fj < fi and ej > ei or fj > fi and ej < ei:
                                scores['cross_%s' % f] += 1.

        print "%s\t%s" % (r, scores)
    except SkipRule:
        pass


    
    

    
