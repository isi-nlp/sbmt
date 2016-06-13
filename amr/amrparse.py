#!/usr/bin/env python
import sys, re, copy
align = re.compile(r'~e\.([0-9,]*)$')
digits = re.compile(r'^\d+$')
def _amr_structure_recurse(itr,nodemap):
    root = next(itr)
    #align = re.compile(r'~e\.([0-9,]*a)$')
    children = []
    assert(root[0] == '/')
    root = root[1:]
    while True:
        key = next(itr)
        if key == ')':
            return (root,children)
        assert(key[0] == ':')
        key = key[1:]
        val = next(itr)
        sval = align.sub('',val)
        if val[0] == '(':
            node = val[1:]
            #nodemap[node] = ('',[])
            croot,cchildren = _amr_structure_recurse(itr,nodemap)
            assert(nodemap[node][0] == '' and len(nodemap[node][1]) == 0)
            nodemap[node] = (croot,cchildren)
            children.append((key,(node,True,True)))
        elif sval in nodemap:
            children.append((key,(val,True,False)))
        else:
            if val[0] != '"':
                if val == sval:
                    val = '"' + sval + '"'
                else:
                  try:
                    val = '"' + sval + '"~e.' + align.search(val).group(1) 
                  except:
                    #print >> sys.stderr, 'sval vs val', sval, val
                    raise
            children.append((key,(val,False,False)))

"""
 creates the internal AMR structure. if you want to discard the AMR class,
 in favor of your own data-type, the structure returned here should help.
 
 returns a tuple. the first element is a map of
      node-label ==>  (concept, [(key1,(val1,is_subamr1)), ..., (keyN,(valN,is_subamrN))])
 where valN is also a node-label, if is_subamrN is true, and is a string literal, if is_subamrN is false
 
 the second element of the tuple is the root node-label
 
 example AMR 1:
   (p / participate-01
      :ARG0 (t / team
            :quant (x / 12
                  :ARG2-of (t2 / total-01
                        :ARG1 t)))
      :ARG1~e.6 (c / compete-01))
      
 becomes the map:
 
    { 'x':  ('12', [('ARG2-of', ('t2', True))]), 
      'c':  ('compete-01', []), 
      't2': ('total-01', [('ARG1', ('t', True))]), 
      't':  ('team', [('quant', ('x', True))]), 
      'p':  ('participate-01', [('ARG0', ('t', True)), ('ARG1', ('c', True))])
    }
    
 rooted at 'p'
 
 example AMR 2:
 
     (e / establish-01
           :ARG1 (f2 / fund
                 :purpose (i / innovate-01)
                 :ARG1-of (a / amount-01
                       :ARG2 (m2 / maximum
                             :op1~e.9 (m / monetary-quantity :quant 1000
                                   :unit (d / dollar
                                         :mod (c / country :wiki "United_States"
                                               :name (n / name :op1 "United" :op2 "States")))))))
           :li (x / 1))
           
 becomes the map:
 
    { 'a':  ('amount-01', [('ARG2', ('m2', True))]), 
      'f2': ('fund', [('purpose', ('i', True)), ('ARG1-of', ('a', True))]), 
      'e':  ('establish-01', [('ARG1', ('f2', True)), ('li', ('x', True))]), 
      'd':  ('dollar', [('mod', ('c', True))]), 
      'i':  ('innovate-01', []), 
      'm':  ('monetary-quantity', [('quant', ('1000', False)), ('unit', ('d', True))]), 
      'c':  ('country', [('wiki', ('United_States', False)), ('name', ('n', True))]), 
      'n':  ('name', [('op1', ('United', False)), ('op2', ('States', False))]), 
      'm2': ('maximum', [('op1', ('m', True))]), 
      'x': ('1', [])
    }
    
 rooted at 'e'
"""
def _amr_structure(lst):
    nodemap = {}
    for ll in lst:
        if len(ll) and ll[0] == '(':
            nodemap[ll[1:]] = ('',[])
    assert lst[0][0] == '(', "expected open paren in token %s" % lst[0] 
    root = lst[0][1:]
    nodemap[root] = _amr_structure_recurse(iter(lst[1:]),nodemap)
    return nodemap,root

def _amr_structure_to_string_recurse(nodemap,root,covered):
    if root in covered:
        return root
    else:
        covered.add(root)
        concept,children = nodemap[root]
        ret = '(' + root + ' / ' + concept
        for label,(croot,issubamr) in children:
            ret += ' :' + label + ' '
            if issubamr:
                ret += _amr_structure_to_string_recurse(nodemap,croot,covered)
            else:
                ret += croot
        ret += ' )'
        return ret


def _amr_structure_to_string(nodemap,root):
    covered = set()
    return _amr_structure_to_string_recurse(nodemap,root,covered)

class AMRException(Exception):
    def __init__(self,message):
        super(AMRException,self).__init__(message)

class Node:
    def __init__(self,v):
            self.root = align.sub('',v[0])
            if self.root[0] == '"' and self.root[-1] == '"':
                self.root = self.root[1:-1]
            self.alignment = []
            self.ref = not v[2]
            mtch = align.search(v[0])
            if mtch:
              try:
                self.alignment = [ int(x) for x in mtch.group(1).split(',') ]
              except:
                #print >> sys.stderr, "E: ", v[0]
                raise
            self.is_subamr = v[1]
    def __str__(self):
        return '<Node root: %s, is_subamr: %s >' % (self.root,self.is_subamr)
    def __repr__(self):
        return str(self)
        

class AMR:
    def copy(self):
        sr = amr2str(self,metadata=True)
        print >> sys.stderr, sr
        lr = [sr,'\n','','#']
        ar = list(read(lr))[0]
        return ar
    @classmethod
    def from_token_stream(cls,tokenvec,meta = {}):
        amr = AMR()
        #print >> sys.stderr, tokenvec
        nodemap,root = _amr_structure(tokenvec)
        #print >> sys.stderr, nodemap
        amr._nodemap = {}
        for k,v in nodemap.iteritems():
            amr._nodemap[k] = [Node((v[0],True,True)),[[vv[0],Node(vv[1])] for vv in v[1]]]
        amr.node = Node((root,True,True))
        amr.metadata = copy.deepcopy(meta)
        return amr

    @classmethod
    def from_string(cls,amrstring):
        amr = list(read([amrstring]))[0]
        return amr

    def subamr(self,val):
        amr = AMR()
        amr._nodemap = self._nodemap
        amr.metadata = self.metadata
        amr.node = val
        return amr

    # manipulators        
    def add_concept(self,conceptname):
        idn = conceptname[0]
        n = 0
        if idn in self._nodemap:
            while idn in self._nodemap:
                n += 1
                idn = '%s%s' % (conceptname[0],n)
        self._nodemap[idn] = [Node((conceptname,True,True)),[]]
        return idn

    def add_connection(self,label,fromnode,tonode):
        if fromnode not in self._nodemap:
            raise AMRException("from-node label '%s' not found in AMR" % fromnode)
        if tonode not in self._nodemap:
            raise AMRException("to-node label '%s' not found in AMR" % tonode)
        # todo: check to see if there is an already printed reference...
        self._nodemap[fromnode][1].append([label,Node((tonode,True,True))])

    def add_literal(self,label,fromnode,value):
        if fromnode not in self._nodemap:
            raise AMRException("from-node label '%s' not found in AMR" % fromnode)
        self._nodemap[fromnode][1].append([label,Node((value,False,False))])

    # tells you if this is a proper sub-AMR, or just a string
    def is_subamr(self):
        return self.node.is_subamr
    
    # tells you if this is just a string
    def is_literal(self):
        return not self.is_subamr()
    
    # if this is a sub-AMR, then returns the node label
    # otherwise, it is just a string literal
    def root_label(self,newvalue=None):
        if newvalue is not None:
            if self.is_subamr():
                raise AMRException("attempted to change the node label of subamr")
            self.node.root = newvalue
        return self.node.root
    def root_alignment(self):
            return self.node.alignment
    def reference(self):
        return self.node.ref
    # returns the root concept (the thingy after the "/" ) of the AMR
    def concept(self,newvalue=None):
        if self.is_literal():
            nd = self.node
        else:
            nd = self._nodemap[self.node.root][0]
        if newvalue is not None:
            nd.root = newvalue
        return nd.root
    def remove_child(self,x):
        del self._nodemap[self.node.root][1][x]
    def alignment(self):
        if self.is_literal():
            return self.node.alignment
        else:
            return self._nodemap[self.node.root][0].alignment
    # a generator. returns a sequence of (:label, sub-AMR) tuples
    def children(self):
        if not self.is_literal():
            for k,v in self._nodemap[self.node.root][1]:
                yield (k,self.subamr(v))
    
    def concepts(self):
        for k in self._nodemap:
            yield self.subamr(Node((k,True,True)))
            
    def __str__(self):
        if self.is_literal():
            s = '"' + self.node.root + '"'
            if self.node.alignment:
                s += '~e.' + ','.join(str(x) for x in self.node.alignment)
            return s
        else:
            return amr2str(self)
            #return _amr_structure_to_string(self._nodemap,self._root)

    def __repr__(self):
        return self.__str__()


# an example function to see the interface of AMR. AMR can be printed automatically.
def amr2str(amr,metadata=False,indent=0):
    def recurse(amr,covered,indent,totalindent=0):
        root = amr.root_label()
        if amr.is_literal():
            return str(amr)
        elif root in covered:
            ret = root
            if len(amr.root_alignment()) > 0:
                ret += '~e.' + ','.join(str(x) for x in amr.root_alignment())
            return ret
        elif len(amr.root_alignment()) > 0:
            ret = root
            ret += '~e.' + ','.join(str(x) for x in amr.root_alignment())
            return ret
        elif amr.reference():
            return root
        else:
            covered.add(root)
            ret = '(' + root
            if indent == 0:
                ret += ' /' + amr.concept()
            else:
                ret += ' / ' + amr.concept() 
            if amr.alignment():
                ret += '~e.' + ','.join(str(x) for x in amr.alignment())
            amrchildren = list(amr.children())
            indentline = False
            if len(amrchildren) > 2:
                indentline = True
            for label,camr in amrchildren:
                if camr.is_subamr():
                    indentline = True
            if indentline and indent > 0:
                totalindent += indent
                sep= '\n' + (' ' *totalindent)
            else:
                sep = ' '
            for label,camr in amrchildren:
                 ret += sep + ':' + label + ' ' + recurse(camr,covered,indent,totalindent)
            if indent == 0:
                ret += ' )'
            else:
                ret += ')'
            return ret
    covered = set()
    ret = ''
    if metadata:
        for k,v in amr.metadata.iteritems():
            ret += '# ::%s %s\n' % (k,v)
    ret += recurse(amr,covered,indent)
    if metadata or indent > 0:
        ret += '\n'
    return ret


# reads a file of amrs, and generates AMR classes
def read(filestream):
    def readmeta(line):
        v = line.strip().split()
        k = ''
        vs = []
        for vv in v:
            if len(vv) > 2 and vv[0:2] == '::':
                if k:
                    if vs:
                        yield k,' '.join(vs)
                    else:
                        yield k,''
                k = vv[2:]
                vs = []
            else:
                vs.append(vv)
        if k:
            if vs:
                yield k,' '.join(vs)
            else:
                yield k,''
    
    paren = re.compile('^(.*?)(\)+)$')
    idre = re.compile('::id ([^ ]*)')
    tokre = re.compile('::tok (.*)')
    sntre = re.compile('::snt (.*)')
    
    bindinst = re.compile(' / ')
    align = re.compile(r'~e\.([0-9,]*)$')
    esc = True
    found = False
    vec = []
    inquote = False
    meta = {}
    def append(vec,val,inquote):
        mtch = align.search(val)
        if inquote:
            nv = align.sub('',val)
            if nv[-1] == '"':
                val = nv
            vec[-1] += ' ' + val
        else:
            val = align.sub('',val)
            vec.append(val)
        if val[0] == '"':
            inquote = True
        if val[-1] == '"':
            inquote = False
        if mtch:
                vec[-1] += mtch.group(0)
        return inquote
    
    for line in filestream:
        
        line = line.strip()
        try:
            if line == '':
                if len(vec) > 0:
                    yield AMR.from_token_stream(vec,meta)
                    meta = {}
                vec = []
            elif line[0] == '#':
                if len(vec) > 0:
                    yield AMR.from_token_stream(vec,meta)
                    meta = {}
                vec = []
                for mk,mv in readmeta(line):
                    meta[mk] = mv
            else:
                v = bindinst.sub(' /',line).strip().split()
                
                for vv in v:
                    m = paren.match(vv)
                    if m:
                        if len(m.group(1)):
                            inquote = append(vec,m.group(1),inquote)
                        for mm in m.group(2):
                            inquote = append(vec,mm,inquote)
                    else:
                        inquote = append(vec,vv,inquote)
        except IOError:
            break
    if len(vec) > 0:
        yield AMR.from_token_stream(vec,meta)

def manipulation_example():
    amrstr = '(x4 / exhibit-01 :arg0 (x1 / express-03 :arg2 (x2 / protein :name (x3 / name :op1 "erbb3" ) ) ) :arg1 (x7 / act-02 :arg0 (x5 / enzyme :name (x6 / name :op1 "pi" :op2 "3" :op3 "kinase" ) ) ) :arg1-of (x11 / depend-01 :degree (x8 / large ) :arg1 (x9 / protein :name (x10 / name :op1 "egf" ) ) ) )'
    print >> sys.stderr, "AMR:\n", amrstr
    amr = AMR.from_string(amrstr)
    print >> sys.stderr, 'adding :xref (x /xref :url "someurl" ) into node x9, step by step'
    print >> sys.stderr, 'add concept /xref'
    x = amr.add_concept('xref')
    print >> sys.stderr, 'connect (x /xref) into node x9 with label :xref'
    amr.add_connection('xref','x9',x)
    print >> sys.stderr, 'add :url "someurl" into (x /xref)'
    amr.add_literal('url',x, "someurl")
    print >> sys.stderr, 'new AMR:\n', amr

if __name__ == '__main__':
    manipulation_example()
    for amr in read(sys.stdin):
        try:
            #print amr
            print amr2str(amr,metadata=True,indent=6)
        except IOError:
            break
