import re
import sys
import exceptions
import traceback
import itertools
from guppy import lazy
from guppy.lazy import merge_lists, product_heap, lazylist, memoized, peekable

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

def read_features(s):
    sm = {}
    for m in s.split(','):
        if m:
            (k,v) = m.split(':')
            sm[k] = float(v)
    return sm

def write_features(sm):
    return ','.join([k + ':' + str(sm[k]) for k in sm.keys()])

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
                        try:
                            for f in hyp.children:
                                foc(f)
                        except:
                            pass
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
            try:
                multi = len(f.incoming) > 1 
                if occmap[f]:
                    if nodemap.has_key(f):
                        return "#" + str(nodemap[f])
                    
                    else:
                        nodemap[f] = len(nodemap) + 1
                        s = "#" + str(nodemap[f])
                        parens = True
                if multi:
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
            except AttributeError:
                s = '"' + f + '"'
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
        return ' '.join([str(c) for c in self.children])
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
        return str(self.token)  + " : " + str(self.line[0:10])

def read_forest(s):
    node_ref_expr = re.compile('^#(\d+)(.*)$')
    or_expr = re.compile('^\(OR$')
    estr_expr = re.compile('"([a-z]+)"')
    internal_hyper_edge_expr = re.compile('^\(([-_\w]+)\<(.*)\>')
    terminal_hyper_edge_expr = re.compile('^([-_\w]+)\<(.*)\>')
    nodemap = {}
    line = []
    
    def pop():
        return line.pop(0)
    def curr():
        return line[0]

    def parse(self,s):
        subst = re.compile('#(\d+)0<>')
        s = subst.sub(r"#\1(0<>)",s)
        splitter = re.compile('(\))|\s+')
        ls = re.split(splitter,s)
        lss = []
        for l in ls:
            if l:
                line.append(l)
        n = read_node(pop())
        if len(line) != 0:
            raise InvalidMatch("",line)
        return n
        
    def read_node(token):
        m = re.match(node_ref_expr,token)
        if m:
            if m.group(2):
                #print "call:  readnode(%s, %s)" % (m.group(2), line[:10])
                n = read_node(m.group(2))
                nodemap[m.group(1)] = n
                return n
            else:
                return nodemap[m.group(1)]
        else:
            m = re.match(or_expr,token)
            if m:
                incoming = []
                while curr() != ")":
                    incoming.append(read_hyper_edge(pop()))
                pop()
                return Forest(incoming)
            else:
                return Forest([read_hyper_edge(token)])
    
    def read_hyper_edge(token):
        parent = 0
        children = []
        features = []
        
        m = re.match(internal_hyper_edge_expr,token)
        if m:
            parent = m.group(1)
            features = read_features(m.group(2))
            while curr() != ")":
                m = re.match(estr_expr,curr())
                if m:
                    children.append(m.group(1))
                    pop()
                else:
                    children.append(read_node(pop()))
            pop()
        else:
            m = re.match(terminal_hyper_edge_expr,token)
            if m:
                parent = m.group(1)
                features = read_features(m.group(2))
            else:
                raise InvalidMatch(token,line)
        return HyperEdge(parent,features,children)

    splitter = re.compile('(\))|\s+')
    ls = re.split(splitter,s)
    for l in ls:
        if l:
            line.append(l)
    n = read_node(pop())
    if len(line) != 0:
        raise InvalidMatch("",line)
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
        s = "totalcost=%s derivation={{{%s}}}" % (self.score, self.tree)
        s += ' ' + ' '.join("%s=%s" % (k,self.features[k]) for k in self.features)
        return s
    def __cmp__(self,other):
        return cmp(self.score,other.score)

def kbest(forest,weights):
    forest2lazylist = {}
    
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
        @peekable
        def dummy(s):
            yield s
        try:
            children = (trees_from_hyp(hyp) for hyp in f.incoming)
            return merge_lists(children)
        except AttributeError:
            children = dummy(dummy(KbestResult(0,{},f)))
            return merge_lists(children)
    
    return trees_from_forest(forest)
    
#-------------------------------------------------------------------------------

if __name__ == "__main__":
    weights = read_features("A:0.89,B:.12,unry:0.567,X_B_A:0.2,X_A_A:0.56,Y_A_A:0.12,Y_B_A:0.79,X:0.5,lm:.6,Y:0.21,distortion:0.91,text-length:-.82,Y_X_Y:-.5,X_X_X:0.6")
    f = '#1(OR (1<cost:1,lm:9.2941,TOP:1> #2(OR (10<cost:3,X_Y_Y:1> #3(29<cost:2,text-length:1,Y_A:1> "a" ) #4(29<cost:2,text-length:1,Y_A:1> "a" ) ) (11<cost:6,X_Y_X:1> #4 #5(28<cost:3,text-length:1,X_A:1> "a" ) ) (12<cost:4,X_X_Y:1> #5 #4 ) (2<X_X_X:1,cost:1> #5 #6(28<cost:3,text-length:1,X_A:1> "a" ) ) ) ) (1<cost:1,lm:12.2434,TOP:1> #7(33<cost:-0.5,text-length:1,unry:1> #8(OR (23<cost:4.1,Y_X_Y:1> #5 #4 ) (13<cost:0.1,Y_X_X:1> #5 #6 ) (14<cost:0.1,Y_X_X:1> #6 #5 ) (21<cost:3.1,Y_Y_Y:1> #3 #4 ) ) "c" ) ) (1<cost:1,lm:9.52882,TOP:1> #9(27<cost:5,X:1> #10(6<cost:3,X_B_A:1> #11(31<cost:1,text-length:1,B:1> "b" ) #12(30<cost:1,text-length:1,A:1> "a" ) ) ) ) (1<cost:1,lm:15.2376,TOP:1> #13(33<cost:-0.5,text-length:1,unry:1> #14(OR (23<cost:4.1,Y_X_Y:1> #15(33<cost:-0.5,text-length:1,unry:1> #3 "c" ) #4 ) (13<cost:0.1,Y_X_X:1> #15 #6 ) ) "c" ) ) )'    
    n = [ 'b b c b a cc c'
        , 'b a c b a cc c'
        , 'a a c b a cc c'
        , 'b b c a b cc c'
        , 'a b c b a cc c'
        , 'b a c a b cc c'
        , 'a a c a b cc c'
        , 'a a c b b cc c'
        , 'b b c b b cc c'
        , 'a a c a a cc c'
        ]
    forest = Forest.read(f)
    forest2 = Forest.read(str(forest))
    
    kn = [ k for k in kbest(forest,weights) ]
    print "numkbests:", len(kn)
    print "numnbests:", len(n)
    for k,s in itertools.izip(kn,n):
        #if k != s:
        print k.tree, k.score, '<==>', s


