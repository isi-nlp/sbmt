#!/usr/bin/env pypy
usage="""
1 arg = n : depth of full binary tree
"""

import sys,os
from tree import *
stdout=os.getenv('showall',False)
pretty=os.getenv('pretty',True)
showperm=os.getenv('showperm',pretty)

from dumpx import *
from graehl import *
from collections import deque

n_perm=0
n_perm_itg_bin=0
def print_itg_bin(perm,out=sys.stdout):
    if out is not None and showperm:
        out.write(' '.join(map(str,perm))+' = ')
    d=itg_bin(perm)
    global n_perm,n_perm_itg_bin
    n_perm+=1
    if d is not None:
        n_perm_itg_bin+=1
        if out is not None:
            out.write('%s'%d[2])
    if out is not None:
        out.write('\n')

mon='.'
inv='^'

def contigspan(spans,connect):
    """return None if spans not contig, else return single span = union of nonoverlapping [a,b) spans. (you ensure that they're nonoverlapping!)
    connect is all None initially will reset connect back to None
    connect must be size N+1 for spans over [0...N)"""
    N=len(spans)
    if N==0: return None
    sp=withi(spans)
    sp.sort()
    minsp=sp[0][0]
    mina=minsp[0]
    maxb=sp[-1][0][1]
    lastb=mina
    for (a,b),_ in sp:
        if lastb!=a:
            #dumpx(lastb,a,'\n'.join(map(str,sp)))
            return None,None
        lastb=b
    return [p[1] for p in sp],(mina,maxb)
    #spans=[s[1] for s in spans]
    #sas=[s[0] for s in spans]
    sbs=[s[1] for s in spans]
    maxb=max(sbs)
    #p='?%s?'%len(spans) #TODO: fill in actual permutation
    i=0
    mina=N+1
    mini=-1
    for a,b in spans:
        assert(connect[a] is None)
        connect[a]=(b,i)
        if (a<mina):
            mini=i
            mina=a
        i+=1
    #sep='' if len(spans)<10 else ','
    r=(mina,maxb)
    p=[]
    #mini]
    b=mina
    while b!=maxb:
        c=connect[b]
        if c is None:
            r=None
            for a,_ in spans:
                connect[a]=None
            break
        p.append(c[1])
        connect[b]=None
        b=c[0]
    connect[b]=None
    return p,r

def useless_m(m):
    return False
#m==3
 # size 3 rules add nothing to size 2 compositions - check if true for all odd>1

def extend_bin(m,u,N,connect):
    "pre: u[a][1..m-1]=max [a,b) using m-1 or smaller rules. connect[_]=None"
    st=[]
    mp=m-1
    if mp>1:
        while useless_m(mp):
            mp-=1
    #dumpx(m,u,N,mp)
    i=0
    #lastm=deque()
    m2=m
    while i<N:
        adj=u[i]
        n=adj[mp]
        adj[m]=n
        st.append(n) #shift
        #dump('shift',st)
        if n is None:
            warn('ERROR u[i=%s][mp=%s]=%s'%(i,mp,adj[mp]))
            dump(N,n,adj)
        assert(n is not None)
        assert(n[0] is not None)
        assert(isinstance(n[0],tuple))
        a,b=n[0]
        assert(i==a)
        i=b
        while len(st)>=m2: #reduce
            last=st[-m2:]
            #dump('reduce?',m2,last)
            fs=[s[1] for s in last]
            #fs=last
            rule,cont=contigspan(fs,connect)
            #dump(m2,fs,rule,cont)
            if cont is None:
                m2+=1
                while useless_m(m2):
                    m2+=1
                if m2<=m:
                    continue
                else:
                    m2=2
                    break
            deriv=Node(rule,[s[2] for s in last])
            imin=st[-m2][0][0]
            r=((imin,b),cont,deriv)
            u[imin][m]=r
            del st[-m2:]
            st.append(r)
            #dump('reduced %s max=%s st=%s'%(m2,m,st))
            m2=2 # todo: improve incrementality or do regular synch parse (ea,fa,len) with viterbi score=min-max rule size

def min_bin(p):
    "return (max tuple size needed,Node(mon|inv|int(leaf)) so that max-#children is minimized. may be (2,itg binarization) or higher"
    """
    approach: we could do a chart with all m,[a,b) such that p[a...b) have been parsed with max=m (filling m=2 first, then incrementally adding m=3 ...). but instead we can be greedy:
    u[a,m]=(ispan,pspan,deriv) with max b in ispan=[a,b) such that p[a...b) contig covering pspan, with max rule size=m. also, we no longer care about u[c,m] if a<=c<=u[a,m] (more greedy). can we keep invariant a single path across chart, never caring about [a,b) vs [c,d) overlaps a<c<b<d?
    """
    if len(p)==0: return (0,Node(mon))
    if len(p)==1: return (1,Node(mon,[Node(0)]))
    if False:
        i=itg_bin(p)
        if i is not None: return (2,i[2])
    N=len(p)
    u=[[None,((a,a+1),(p[a],p[a]+1),Node(a))]+[None]*(N-1) for a in range(N)]
    connect=[None]*(N+1)
    for m in range(2,N+1):
        if useless_m(m):
            continue
        extend_bin(m,u,N,connect)
        s=u[0][m]
        if s is not None and s[0][1]==N:
            #dumpx(s,str(s[2]),N)
            return (m,s[2])
    assert(False)
    #return (len(p),Node(mon,map(Node,p)))

def print_stats(out=sys.stderr):
    out.write('%s out of %s permutations (%s) were ITG-binarizable.\n'%(n_perm_itg_bin,n_perm,float(n_perm_itg_bin)/n_perm))


def itg_bin(perm):
    """
    *** input: perm=[0,1,4,2,3] a permutation of [0,1,...,max] - think of this
as a funny word:word alignment if you like (every word is 1:1 aligned)

    *** output: returns None if the original and permuted sequence
[0,1,....,max] can't be synchronously generated by a 1-state ITG (with at most
2 symbols and 1 NT on the source and target rhs) grammar.

     otherwise, return Tree with labels '[]' or '<>' except leaves
    """
    p=[int(x) for x in perm]
#    dump('itg_bin(%s)'%p)
    n=len(p)
    if n==1:
        return Tree(p[0])
    if (n==0 or min(p)!=0 or max(p)!=n-1):
        raise Exception("expected args, or lines to stdin, of at least 1 space-separated indices (permutations of >=1 elements notated e.g. 0 2 1 of the ints 0...max) - you supplied %s min=%s max=%s"%(perm,min(p),max(p)))
    return itg_bin_greedy(p)

#partial itg binarizations: (a,b,tree). covers a..b in p
def connect(a,b):
    if a[1]==b[0]:
        return (a[0],b[1],Node(mon,[a[2],b[2]]))
    elif a[0]==b[1]:
        return (b[0],a[1],Node(inv,[a[2],b[2]]))
    return None

def itg_bin_greedy(p):
    assert(len(p)>1)
    i=0
    s=[]
    for i in range(0,len(p)):
        x=p[i]
        s.append((x,x+1,Node(x)))
        while(len(s)>1):
            c=connect(s[-2],s[-1])
            if c is None:
                break
            s.pop()
            s.pop()
            s.append(c)
    if len(s)==1:
        return s[0]
    dump("can't ITG binarize %s"%' '.join(str(x[2]) for x in s))
    return None


# pypy doesn't support itertools.permutations
def permute_in_place(a):
    'a is a list. yield all shuffled versions of it, starting w/ sorted'
    a.sort()
    yield list(a)
    if len(a) <= 1:
        return
    first = 0
    last = len(a)
    while 1:
        i = last - 1
        while 1:
            i = i - 1
            if a[i] < a[i+1]:
                j = last - 1
                while not (a[i] < a[j]):
                    j = j - 1
                a[i], a[j] = a[j], a[i] # swap
                r = a[i+1:last]
                r.reverse()
                a[i+1:last] = r
                yield list(a)
                break
            if i == first:
                a.reverse()
                return

def mapl(t,f):
    return (mapl(t[0],f),mapl(t[1],f)) if isinstance(t,tuple) else f(t)

def plusl(t,n):
    return mapl(t,lambda x:x+n)

def fullbin(d,n=0):
    if d<=0:
        return n
    return (fullbin(d-1,n),fullbin(d-1,n+2**(d-1)))

def inside(t,path,i=0):
    "return left-outside,t[path],reversed(right-outside) or None if t[path] doesn't exist. t are binary tuples or leaves"
    #dump(t,path,i)
    if i<len(path) and isinstance(t,tuple):
        d=path[i]
        r=inside(t[d],path,i+1)
        #dump(i,r)
        if r is None:
            return None
        a,b,c=r
        if d==1: # could generalize to i<d, i>d
            a.append(t[0])
        else:
            c.append(t[1])
        return (a,b,c)
    return ([],t,[]) if i==len(path) else None

def yieldbin(t):
    return yieldbin(t[0])+yieldbin(t[1]) if isinstance(t,tuple) else [t]

def flattena(t,a):
    if isinstance(t,int): a.append(t)
    else:
        for c in t:
            flattena(c,a)

def flattenr(t):
    a=[]
    flattena(t,a)
    return a

#todo: arbitrary permutation instead of ITG-unbinarizable 2 0 3 1
def xform(t,path):
    r=inside(t,path)
    #dump(t)
    if r is not None and isinstance(r[1],tuple):
        a,(b,c),d=r
        #dump(a,b,c,d)
        # for [0] [1]
        #l=[c]+a+[b]+d # easy
        #l=[b]+a+d+[c] # easy
        l=[b]+a+d+[c] # hard
        #l=[b]+d+a+[c] # hard
        #dump(l)
        return [xform(x,path) for x in l]
    else:
        return yieldbin(t)
show=False

def checkcomp(f,p1,p2):
    #dump(f)
    #dump("a",p1)
    #dump("b",p2)
    #dump(yieldbin(f))
    x1=xform(f,p1)
    #dump(x1)
    x2=xform(f,p2)
    #dump(x2)
    f1=flattenr(x1)
    f2=flattenr(x2)
    f1i=invert_perm(f1)
    #dump(f1)
    cp=compose_perm(f2,f1i) #f1i first - fn composition notation
    if show:
        dump("a",f1)
        dump("a-1",f1i)
        dump("a*a-1",compose_perm(f1,f1i))
        dump("b",f2)
        dump("a-1 * b",cp)
    return cp

def perms(n,p1=[],p2=[1],allperm=False):
    if allperm:
        for p in permute_in_place(range(n)):
            yield p
        return
    f=fullbin(n)
    cp=checkcomp(f,p1,p2)
    yield cp

maxsz=0
maxperm=None
maxplen=None
def maxminbin(p):
    global maxsz,maxperm,maxplen
    m,deriv=min_bin(p)
    if m>maxsz:
        maxplen=len(p)
        d=deriv if show else ''
        print 'new max SCFG rule size for perm of %s items: %s %s'%(len(p),m,d)
        maxsz=m
        maxperm=deriv

import optfunc
@optfunc.arghelp('rest_','a permutation of 0...n-1')
def main(rest_=[],a="0",b="1",depth=3,itg=False,smallest=True,max_depth=0,allperm=False,showperm=True):
    #dumpx(compose_perm([1,0,2],[2,1,0]))
    global show
    show=showperm
    logcmd(True)
    if len(rest_):
        maxminbin(map(int,rest_))
    if not max_depth:
        max_depth=depth
    rule=[map(int,x.split()) for x in [a,b]]
    for n in range(depth,max_depth+1):
        for p in perms(n,rule[0],rule[1],allperm):
            if showperm: dump("perm n=%s: "%n,p)
            if itg:
                print_itg_bin(p)
            if smallest:
                maxminbin(p)
    if smallest:
        mp=maxperm if showperm else ''
        print "Max of min-size-rule binarization: %s for len %s perm%s"%(maxsz,maxplen,mp)

optfunc.main(main)
