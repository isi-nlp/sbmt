import re, itertools, sys

lhsre = re.compile(r'^([^()]*)')
lhsvar = re.compile(r'x(\d+):([^\s()]*)')
rhsvar = re.compile(r'\sx(\d+)')
lhsrhs = re.compile(r'(.*)\s+->(.*)')
hwlexpos = re.compile(r'hwpos=\{\{\{"(.*)"\}\}\}')
hwidxpos = re.compile(r'hwpos=\{\{\{([0-9]+)\}\}\}')
# 107108324
def unflatten_pairs(lst):
    x = 0
    p = ''
    for n in lst:
        x += 1
        if x % 2 == 0:
            yield p,int(n)
        else:
            p = n

class hw_table:
    class hpos:
        def __init__(self,hwpos,rhs):
            hmatch = hwlexpos.match(hwpos)
            if hmatch:
                self.is_lex = True
                self.word = hmatch.group(1)
            else:
                self.is_lex = False
                d = rhs[int(hwidxpos.match(hwpos).group(1))]
                self.root = d.root
                self.distribution = dict(unflatten_pairs(d.distribution.split()[1:]))
    class dist:
        def __init__(self,nt,line):
            self.root = nt
            self.distribution = line
    def __init__(self,line):
      try:
        v = line.strip().split('\t')
        self.lhs_root = lhsre.match(v[0]).group(1)
        self.rule = v[0]
        self.count = int(v[3].strip())
        ls,rs = lhsrhs.match(v[0]).group(1,2)
        lhs = dict( (int(m.group(1)),m.group(2)) for m in lhsvar.finditer(ls) )
        self.rhs = dict( (int(m.group(1)),hw_table.dist(lhs[int(m.group(1))],n)) \
                         for m,n in itertools.izip(rhsvar.finditer(rs),v[5:]) \
                       )
        self.hwpos = hw_table.hpos(v[1],self.rhs)
      except:
        print >> sys.stderr, line
        raise


def wb_prob(n,c,p,b,m):
    if c == 0 or n == 0:
        return b
    wp = float(n) / (float(n) + m*float(c))
    wb = float(m*float(c)) / (float(n) + m*float(c))
    return wp * p + wb * b

        
