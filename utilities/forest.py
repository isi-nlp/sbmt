import re
import sys
import exceptions
import traceback
from guppy import lazy
from guppy.lazy import merge_lists, product_heap, lazylist, memoized, peekable
from guppy import progress
import itertools

"""
def inner_prod(m1,m2):
    if len(m2) < len(m1):
        return inner_prod(m2,m1)
    else:
        d = 0
        for k in m1.keys():
             if m2.has_key(k):
                 d += m1[k] * m2[k]
        return d


def add_features(m1,m2):
    if len(m2) > len(m1):
        return add_features(m2,m1)
    m = m1.copy()
    for k in m2.keys():
        if m.has_key(k):
            m[k] += m2[k]
        else:
            m[k] = m2[k]
    return m

"""

def inner_prod(ff1,ff2):
    f1 = feature_as_array(ff1)
    f2 = feature_as_array(ff2)
    i1 = 0
    i2 = 0
    ret = 0
    while i1 != len(f1) and i2 != len(f2):
        c = cmp(f1[i1][0],f2[i2][0])
        if c == 0:
            ret += f1[i1][1] * f2[i2][1]
            i1 += 1
            i2 += 1
        elif c < 0:
            i1 += 1
        else:
            i2 += 1
    return ret

def add_features(ff1,ff2):
    f1 = feature_as_array(ff1)
    f2 = feature_as_array(ff2)
    retp = [ x for x in itertools.chain(f1,f2) ]
    retp.sort()
    ret = []
    if len(retp)  == 0: return retp
    k = retp[0][0]
    v = retp[0][1]
    for p in retp:
        if p[0] == k:
            v += p[1]
        else:
            ret.append((k,v))
            k = p[0]
            v = p[1]
    ret.append((k,v))
    return feature_as_string(ret)
    
def read_features(s):
    return s

def write_features(s):
    return s

def feature_as_array(s):
    sm = []
    #print >> sys.stderr, "feature-vec", s
    for m in s.split(','):
        if m:
            #print >> sys.stderr, "feature", m
            (k,v) = m.split(':')
            sm.append((k,float(v)))
    sm.sort()
    return sm

def feature_as_string(sm):
    return ','.join(k + ':' + str(v) for (k,v) in sm)
    


# pre-load: 25.8 MB
# with component scores: 59.5 MB (+33.7 MB)
# without component scores: 34.9 MB (+9.1 MB)
# with component scores represented as strings: 39.2 MB (+13.4)
class HyperEdge:
    def __init__(self,parent,features,children):
        self.parent = parent
        self.features = features
        self.children = list(children)

class Forest:
    @classmethod
    def read(cls,s):
        return read_forest(s)
    
    def __init__(self,incoming = []):
        self.incoming = list(incoming)
    
    def __str__(self):
        def forest_shared_map(forest):
            occmap = {}
            def foc(forest):
                if occmap.has_key(forest):
                    occmap[forest] = True
                else:
                    occmap[forest] = False
                    for hyp in forest.incoming:
                        for f in hyp.children:
                            foc(f)
            foc(forest)
            return occmap
        
        occmap = forest_shared_map(self)
        nodemap = {}
        def _hstr(h):
            s = str(h.parent) + "<" + write_features(h.features) + ">"
            if len(h.children):
                s = "(" + s
                s += ''.join([' '+_str(child) for child in h.children])
                s += ")"
            return s
        def _str(f):
            close = False
            parens = False
            s = ""
            if occmap[f]:
                if nodemap.has_key(f):
                    return "#" + str(nodemap[f])
                    
                else:
                    nodemap[f] = len(nodemap) + 1
                    s = "#" + str(nodemap[f])
                    parens = True
            if len(f.incoming) > 1:
                s += "(OR"
                s += ''.join([(' '+_hstr(h)) for h in f.incoming])
                s += ")"
            else:
                parens = parens and (len(f.incoming[0].children) == 0)
                if parens:
                    s += "("
                s += _hstr(f.incoming[0])
                if parens:
                    s += ")"
            return s
        return _str(self)

class Tree:
    @classmethod
    def read(cls,s):
        return read_tree(s)
        
    def __init__(self,parent,children):
        self.parent = parent
        self.children = list(children)
    def __str__(self):
        if len(self.children) == 0:
            return str(self.parent)
        return "(" + str(self.parent) + ''.join([' ' + str(c) for c in self.children]) + ")"
    def __repr__(self):
        return str(self)

def read_tree(s):
    compound_expr = re.compile('^\(([-_\w]+)$')
    terminal_expr = re.compile('^[-_\w]+$')
    line = []

    def curr():
        return line[0]
        
    def pop():
        return line.pop(0)
    def read(token):
        parent = ""
        children = []
        m = re.match(compound_expr,token)
        if m:
            parent = m.group(1)
            children = []
            while curr() != ")":
                tok = pop()
                children.append(read(tok))
            pop()
        else:
            m = re.match(terminal_expr,token)
            if m:
                parent = token
            else:
                raise InvalidMatch(token,line)
        return Tree(parent,children)
    
    ls = re.split('(\))|\s+',s)
    lss = []
    for l in ls:
        if l:
            line.append(l)
    tok = pop()
    n = read(tok)
    if len(line) != 0:
        raise InvalidMatch("",line[1:])
    return n
        
class InvalidMatch(exceptions.Exception):
    def __init__(self,token,line):
        self.token = token
        self.line = line
    def __str__(self):
        remainder = []
        x = 0
        for s in self.line:
            if x == 10: break
            x += 1
            remainder.append(s)
        return '"' + str(self.token) + '"'  + " : " + ' '.join('"' + ss + '"' for ss in remainder)

def read_forest(s):
    
    splitter = None
    pgs = progress.ProgressMeter(total=10)
    node_ref_expr = re.compile('^#(\d+)(.*)$')
    or_expr = re.compile('^\(OR$')
    internal_hyper_edge_expr = re.compile('^\(([-_\w]+)\<(.*)\>')
    terminal_hyper_edge_expr = re.compile('^([-_\w]+)\<(.*)\>')
    nodemap = {}

    
    def pop():
        #print >> sys.stderr, splitter.peek(),
        return splitter.next()
    def curr():
        return splitter.peek()

    def read_node(token):
        if token[0] == '#':
            m = re.match(node_ref_expr,token)
            if m.group(2):
                #print "call:  readnode(%s, %s)" % (m.group(2), line[:10])
                n = read_node(m.group(2))
                nodemap[m.group(1)] = n
                return n
            else:
                return nodemap[m.group(1)]
        elif token == "(OR":
            incoming = []
            while curr() != ")":
                stro = pop()
                if stro == ")": raise WhatTheHell()
                incoming.append(read_hyper_edge(stro))
            pop()
            return Forest(incoming)
        else:
            return Forest([read_hyper_edge(token)])
    
    def read_hyper_edge(token):
        parent = 0
        children = []
        features = None
        
        if token[0] == '(':
            m = re.match(internal_hyper_edge_expr,token)
            parent = m.group(1)
            features = read_features(m.group(2))
            while curr() != ")":
                children.append(read_node(pop()))
            pop()
        else:
            m = re.match(terminal_hyper_edge_expr,token)
            if m:
                parent = m.group(1)
                features = read_features(m.group(2))
            else:
                raise InvalidMatch(token,splitter)
        return HyperEdge(parent,features,children)
    
    @peekable
    def split_line(s):
        toka = []
        for c in s:
            if c != ' ' and c != '\n':
                toka.append(c)
            elif len(toka) > 0:
                yield ''.join(toka)
                toka = []
        if len(toka) > 0:
            yield ''.join(toka)
    

    splitter = split_line(s)

    n = read_node(pop())
#    if len(line) != 0:
#        raise InvalidMatch("",line)
    return n

def intersect_hyperedge_tree(hyperedge,tree):
    if hyperedge.parent == tree.parent:
        if len(hyperedge.children) == len(tree.children):
            if len(tree.children) == 0:
                return hyperedge,True
            else:
                p = []
                isv = True
                for x in xrange(0,len(tree.children)):
                    f = intersect_forest_tree(hyperedge.children[x],tree.children[x])
                    if len(f.incoming) == 0:
                        isv = False
                        break
                    else:
                        p.append(f)
                if isv:
                    return HyperEdge(hyperedge.parent,hyperedge.features,p),True
    return hyperedge,False

def explode_forest(forest,skip):
    oldnewmap = {}
    def gen_product(prod,f,g):
        for fx in f:
            for gx in g:
                yield prod(fx,gx)
                
    def write_ornode(o):
        return '(' + ','.join(x.parent for x in o.incoming) + ')'
        
    def write_ornode_list(ls):
        return '[' + ' '.join(write_ornode(x) for x in ls) + ']'
        
    def write_explosion(prechildren,gc):
        s = " X ".join([' '.join(['<' + write_features(x[0]) + '>.' + write_ornode_list(x[1]) for x in pc]) for pc in prechildren])
        s += ' => '
        s += ' '.join(['<' + write_features(x[0]) +'>.' + write_ornode_list(x[1]) for x in gc])
        s += '\n'
        return s
        
    def explosion(hyper):
        prechildren = [list(ornode_explosion(f)) for f in hyper.children]
        gc = [({},[])]
        for pc in prechildren:
            gnc = []
            for x in gc:
                for y in pc:
                    #print >> sys.stderr, "cocat: ", x, " , ", y
                    gnc.append((add_features(x[0],y[0]), x[1][:] + y[1][:]))
            gc = gnc
            #gc = [(add_features(x[0],y[0]), x[1]+y[1]) for y in pc for x in gc ]
                #assert pc == gc, str(pc) + " != " + str(gc) + ": sanity = "+str(sanity)
        return gc
        
    def andnode_explosion(andnode):
        assert skip(andnode.parent)
        if len(andnode.children) == 0:
            yield (andnode.features,[])
        else:
            gc = explosion(andnode)
            for pc in gc:
                yield (add_features(andnode.features,pc[0]),pc[1])
    
    def ornode_explosion(ornode):
        assert len(ornode.incoming) != 0

        if skip(ornode.incoming[0].parent):
            for hyp in ornode.incoming:
                for a in andnode_explosion(hyp):
                    yield a
        else:
            yield ({},[forest_explosion(ornode)])
    
    def hyperedge_explosion(hyper):
        assert not skip(hyper.parent)
        if len(hyper.children) == 0:
            yield hyper
        # children is list of (feature,children)
        else:
            gc = explosion(hyper)
            for pc in gc:
                yield HyperEdge(hyper.parent,add_features(hyper.features,pc[0]),pc[1])
                
    class MixedSkippingForest(exceptions.Exception):
        def __str__(self):
            return "some hyperedges in non-skip forest node are skippable"
    
    def forest_explosion(forest):
        for hyp in forest.incoming:
            if skip(hyp.parent):
                raise MixedSkippingForest()
        if oldnewmap.has_key(forest):
            return oldnewmap[forest]
        else:
            f = Forest((h for hyp in forest.incoming for h in hyperedge_explosion(hyp)))
            oldnewmap[forest] = f
            return f
    
    return forest_explosion(forest)

#-------------------------------------------------------------------------------

def intersect_forest_tree(forest,tree):
    hyps = []
    for hyper in forest.incoming:
        h,valid = intersect_hyperedge_tree(hyper,tree)
        if valid:
            hyps.append(h)
    return Forest(hyps)
    
#-------------------------------------------------------------------------------

def explode_tree(tree,skip):
    def tree_explosion(tree,skip):
        children = (cc for c in tree.children for cc in tree_explosion(c,skip))
        if skip(tree.parent):
            for c in children:
                yield c
        else:
            yield Tree(tree.parent,children)
    class RootSkipped(exceptions.Exception):
        def __str__(self):
            return "cannot skip root node in expand_tree"
    if skip(tree.parent):
        raise RootSkipped()
    return tree_explosion(tree,skip).next()
    
#-------------------------------------------------------------------------------

class KbestResult(object):
    def __init__(self,score,weights,tree):
        self.score = score
        self.features = weights
        self.tree = tree
    def __str__(self):
        s = "totalcost=%g derivation={{{%s}}} " % (self.score, self.tree)
        f = feature_as_array(self.features)
        s += ' '.join("%s=%g" % p for p in f)
        return s
    def __cmp__(self,other):
        return cmp(self.score,other.score)
        
def sort_forest(forest,weights):
    forest_scores = {}
    
    def forest_score(forest):
        if forest in forest_scores:
            return forest_scores[forest]
        newincoming = [ (hyp_score(hyp),hyp) for hyp in forest.incoming ]
        newincoming.sort()
        #newincoming.reverse()
        #print >> sys.stderr, "scores:", ' '.join(`x[0]` for x in newincoming)
        forest.incoming = [ x[1] for x in newincoming ]
        forest_scores[forest] = newincoming[0][0]
        return newincoming[0][0]
        
    def hyp_score(hyp):
        score = inner_prod(hyp.features,weights)
        for child in hyp.children:
            score += forest_score(child)
        return score
    forest_score(forest)


def kbest(forest,weights):
    forest2lazylist = {}
    sort_forest(forest,weights)
    @peekable
    def trees_from_hyp(hyp):
        score = inner_prod(hyp.features,weights)
        def make_tree(*children):
            s = score
            v = hyp.features
            for c in children:
                s += c.score
                v = add_features(v,c.features)
            return KbestResult(s,v,Tree(hyp.parent,(c.tree for c in children)))
        if len(hyp.children) == 0:
            yield KbestResult(score,hyp.features,Tree(hyp.parent,[]))
        else:
            children = []
            for f in hyp.children:
                if f in forest2lazylist:
                    children.append(forest2lazylist[f])
                else:
                    ll = lazylist(trees_from_forest(f))
                    children.append(ll)
                    forest2lazylist[f] = ll
            for h in product_heap(*children,**{'map':make_tree}):
                yield h
    
    def trees_from_forest(f):
        children = (trees_from_hyp(hyp) for hyp in f.incoming)
        return merge_lists(children)
  
    return trees_from_forest(forest)
    
#-------------------------------------------------------------------------------

if __name__ == "__main__":
    weights = read_features('lm:1,p2:1')
    f = "(OR (100<lm:5.30237,p2:1,p1:1> (OR (6<lm-unk:1,p2:6,text-length:1,p1:1,pa:1> 5<lm:3.21615,lm-unk:1,p2:5,text-length:3,p1:1,pb:1,pc:1,pd:1>) (7<lm-unk:1,p2:7,p1:1> 13<p2:11,text-length:1,p1:1,pa:1> 5<lm:3.21615,lm-unk:1,p2:5,text-length:3,p1:1,pb:1,pc:1,pd:1>))) (100<lm:1.62994,lm-unk:1,p2:1,p1:1> (7<lm-unk:1,p2:7,p1:1> 4<lm:6.47364,p2:4,text-length:3,p1:1,pa:1,pb:1,pc:1> 2<p2:2,text-length:1,p1:1,pd:1>)) (100<lm:1.38868,lm-unk:1,p2:1,p1:1> (OR (7<lm:3.62547,p2:7,p1:1> (OR (6<lm-unk:1,p2:6,text-length:1,p1:1,pa:1> 3<lm-unk:1,p2:3,text-length:2,p1:1,pb:1,pc:1>) (7<lm-unk:1,p2:7,p1:1> 13<p2:11,text-length:1,p1:1,pa:1> 3<lm-unk:1,p2:3,text-length:2,p1:1,pb:1,pc:1>)) 2<p2:2,text-length:1,p1:1,pd:1>) (6<lm-unk:1,p2:6,text-length:1,p1:1,pa:1> #9(7<lm:3.62547,p2:7,p1:1> 3<lm-unk:1,p2:3,text-length:2,p1:1,pb:1,pc:1> 2<p2:2,text-length:1,p1:1,pd:1>)) (7<lm-unk:1,p2:7,p1:1> (12<p2:10,p1:1,pb:1,pc:1> 13<p2:11,text-length:1,p1:1,pa:1>) 2<p2:2,text-length:1,p1:1,pd:1>) (7<lm-unk:1,p2:7,p1:1> 13<p2:11,text-length:1,p1:1,pa:1> #9))) (100<lm:1.04671,lm-unk:1,p2:1,p1:1> (OR (7<lm:2.78037,p2:7,p1:1> (7<lm-unk:1,p2:7,p1:1> 10<lm:3.62547,p2:8,text-length:2,p1:1,pa:1,pb:1> 11<p2:9,text-length:1,p1:1,pc:1>) 2<p2:2,text-length:1,p1:1,pd:1>) (7<lm-unk:1,p2:7,p1:1> 10<lm:3.62547,p2:8,text-length:2,p1:1,pa:1,pb:1> (7<lm:2.78037,p2:7,p1:1> 11<p2:9,text-length:1,p1:1,pc:1> 2<p2:2,text-length:1,p1:1,pd:1>)))) (101<lm:5.30237,p2:1,p1:1> (8<lm-unk:2,p2:7,p1:1> 13<p2:11,text-length:1,p1:1,pa:1> 3<lm-unk:1,p2:3,text-length:2,p1:1,pb:1,pc:1> 2<p2:2,text-length:1,p1:1,pd:1>)) (101<lm:3.49218,p2:1,p1:1> (8<lm-unk:2,p2:7,p1:1> 10<lm:3.62547,p2:8,text-length:2,p1:1,pa:1,pb:1> 11<p2:9,text-length:1,p1:1,pc:1> 2<p2:2,text-length:1,p1:1,pd:1>)))"
    d = "(100 (7 (12 13) 2))"
    forest = Forest.read(f)
    forest2 = Forest.read(str(forest))
    
    for k in kbest(forest,weights):
        print k
    deriv = Tree.read(d)
    print deriv
    print forest
    print forest2

    print intersect_forest_tree(forest,deriv)
    x = kbest(intersect_forest_tree(forest,deriv),weights).next()
    print x
    vderiv = Tree.read("(100 (V (7 (12 (V 13))) 2))")
    print "expand tree:", explode_tree(vderiv, lambda x: x == "V")
    
    bf = "(OR (100<lm:5.30237,p2:1,p1:1> (OR (6<lm-unk:1,p2:6,text-length:1,p1:1> #1(0<pa:1> 0<>) #2(5<p2:5,p1:1> (0<lm:3.21615,lm-unk:1,text-length:3> #3(0<pb:1> 0<>) #4(0<pc:1> 0<>)) #5(0<pd:1> 0<>))) (7<lm-unk:1,p2:7,p1:1> #6(13<p2:11,text-length:1,p1:1> #1) #2))) (100<lm:1.62994,lm-unk:1,p2:1,p1:1> (7<lm-unk:1,p2:7,p1:1> (4<p2:4,p1:1> (0<lm:6.47364,text-length:3> #1 #3) #4) #8(2<p2:2,text-length:1,p1:1> #5))) (100<lm:1.38868,lm-unk:1,p2:1,p1:1> (OR (7<lm:3.62547,p2:7,p1:1> (OR (6<lm-unk:1,p2:6,text-length:1,p1:1> #1 #7(3<lm-unk:1,p2:3,text-length:2,p1:1> #3 #4)) (7<lm-unk:1,p2:7,p1:1> #6 #7)) #8) (6<lm-unk:1,p2:6,text-length:1,p1:1> #1 #9(7<lm:3.62547,p2:7,p1:1> #7 #8)) (7<lm-unk:1,p2:7,p1:1> (12<p2:10,p1:1> (0<> #6 #3) #4) #8) (7<lm-unk:1,p2:7,p1:1> #6 #9))) (100<lm:1.04671,lm-unk:1,p2:1,p1:1> (OR (7<lm:2.78037,p2:7,p1:1> (7<lm-unk:1,p2:7,p1:1> #11(10<lm:3.62547,p2:8,text-length:2,p1:1> #1 #3) #10(11<p2:9,text-length:1,p1:1> #4)) #8) (7<lm-unk:1,p2:7,p1:1> #11 (7<lm:2.78037,p2:7,p1:1> #10 #8)))) (101<lm:5.30237,p2:1,p1:1> (8<lm-unk:1,p2:7,p1:1> (0<lm-unk:1> #6 #7) #8)) (101<lm:3.49218,p2:1,p1:1> (8<lm-unk:1,p2:7,p1:1> (0<lm-unk:1> #11 #10) #8)))"
    print "exploded-forest:"
    ef = explode_forest(Forest.read(bf),lambda x : x == "0")
    d0 = Tree.read("(100 (6 5))")
    d1 = Tree.read("(100 (7 4 2))")
    d2 = Tree.read("(100 (7 (6 3) 2))")
    
    print "f0: ", intersect_forest_tree(ef,d0)
    print "f1: ", intersect_forest_tree(ef,d1)
    print "f2: ", intersect_forest_tree(ef,d2)
    
    feats = read_features("text-length:1.0")
    feats = add_features(feats,{})
    feats = add_features({},feats)
    print write_features(feats)
    
