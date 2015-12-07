import sys, math
import collections
import getopt
import simplerule
import svector

inverse = False
threshold = 2.
floor = 1e-9
opts, args = getopt.getopt(sys.argv[1:], "it:f:")
for o, a in opts:
    if o == "-i":
        inverse = True
    elif o == "-t":
        threshold = float(a)
    elif o == "-f":
        floor = float(a)

filename, feat = args
            
input = []
vocab = set()
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
                vocab.add((f,e))
                vocab.add((f,"*lambda*"))
        else:
            vocab.add(("NULL", e))

tags = set()
ttable = collections.defaultdict(float)
for line in file(filename):
    (tag, f, e, p) = line.split()
    if tag != "*":
        tags.add(tag)
    if (f,e) in vocab:
        ttable[tag,f,e] = float(p)

def compute(tag, erhs, ealign):
    p = 1.
    for ei,e in enumerate(erhs):
        if isinstance(e, simplerule.Nonterminal):
            continue
        if len(ealign[ei]) == 0:
            fs = ["NULL"]
        else:
            fs = ealign[ei]
        s = 0.
        for f in fs:
            if (tag,f,"*lambda*") not in ttable:
                s += ttable["*",f,e]
            else:
                lam = ttable[tag,f,"*lambda*"]
                s += lam*ttable[tag,f,e] + (1-lam)*ttable["*",f,e]
        s /= len(fs)
        if s == 0.:
            #sys.stderr.write("failed to look up probability for %s\n" % e)
            s = floor
        p *= s
    return p
    
for rulestr, scores, erhs, ealign in input:
    v = svector.Vector()
    p_backoff = compute("*", erhs, ealign)
    try:
        v[feat] = -math.log10(p_backoff)
    except:
        sys.stderr.write("couldn't compute lexical features for rule %s (p=%s), dropping\n" % (rulestr, p_backoff))
        continue
        
    for tag in tags:
        p = compute(tag, erhs, ealign)
        r = p/p_backoff
        if r <= 1./threshold or r >= threshold:
            v["%s_%s" % (feat, tag)] = -math.log10(r)

    # save space
    vstr = " ".join("%s=%.2f" % (x,y) for (x,y) in v.iteritems())

    print "%s\t%s %s" % (rulestr, scores, vstr)
