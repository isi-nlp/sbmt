import sys, math
import getopt
sys.path.append('/home/nlg-01/chiangd/hiero-hadoop/')
import simplerule
import svector

inverse = False
floor = 1e-9
opts, args = getopt.getopt(sys.argv[1:], "if:")
for o, a in opts:
    if o == "-i":
        inverse = True
    elif o == "-f":
        floor = float(a)

filename, feat = args
            
input = []
ttable = {}
for line in sys.stdin:
    rulestr, scores = line.rstrip().split('\t')
    rule = simplerule.Rule.from_str(rulestr)

    try:
        align = rule.attrs['align']
    except KeyError:
        sys.stderr.write("rule missing align: %s\n" % str(rule))
        raise
    align = [tuple(int(i) for i in s.split('-',1)) for s in align.split()]
    frhs, erhs = rule.frhs, rule.erhs

    if inverse:
        frhs, erhs = erhs, frhs
        align = [(ei, fi) for (fi, ei) in align]
        
    ealign = [[] for ei in xrange(len(erhs))]
    for fi,ei in align:
        ealign[ei].append(frhs[fi])

    input.append((rulestr, scores, erhs, ealign))

    for ei, e in enumerate(erhs):
        if isinstance(e, simplerule.Nonterminal): continue
        if len(ealign[ei]) > 0:
            for f in ealign[ei]:
                ttable[f,e] = 0.
        else:
            ttable["NULL",e] = 0.

sys.stderr.write("reading ttable from %s\n" % filename)
for li,line in enumerate(file(filename)):
    (e, f, p) = line.split()
    if (f,e) in ttable:
        ttable[f,e] = float(p)
    if (li+1) % 10000000 == 0:
        sys.stderr.write("  read %d lines\n" % (li+1))

def compute(erhs, ealign):
    p = 1.
    for ei,e in enumerate(erhs):
        if isinstance(e, simplerule.Nonterminal):
            continue
        if len(ealign[ei]) == 0:
            fs = ["NULL"]
        else:
            fs = ealign[ei]
        p *= max(floor,sum(ttable.get((f,e),0.) for f in fs)/len(fs))
    return p
    
for rulestr, scores, erhs, ealign in input:
    v = svector.Vector()
    p = compute(erhs, ealign)
    try:
        v[feat] = -math.log10(p)
    except:
        sys.stderr.write("couldn't compute lexical features for rule %s (p=%s), dropping\n" % (rulestr, p))
        continue
        
    print "%s\t%s %s" % (rulestr, scores, v)

    
