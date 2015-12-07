import math
import bisect

class ThisShouldNotHappen(Exception):
    pass

class Buffer(object):
    def __init__(self, xs, weight):
        self.weight = weight
        self.xs = xs

    def __str__(self):
        return "%s weight %s" % (self.xs, self.weight)

collapse_flag = True
def collapse(bufs):
    weight=sum(buf.weight for buf in bufs)

    if weight % 2 == 1:
        offset = (weight + 1)/2 - 1
    else:
        global collapse_flag
        if collapse_flag:
            offset = weight/2 - 1
        else:
            offset = (weight + 2)/2 - 1
        collapse_flag = not collapse_flag

    xs = [(x,buf.weight) for buf in bufs for x in buf.xs]
    xs.sort()

    ys = []

    i = 0
    for (x,xweight) in xs:
        i1 = i % weight
        if i1 > offset:
            i1 -= weight
        if i1 <= offset < i1 + xweight:
            ys.append(x)
        i += xweight

    return Buffer(ys, weight)


def adjust_phi(phi, n, beta_n):
    beta = float(beta_n)/n
    return (2*phi+beta-1)/(2*beta)

def compute_bk(n, eps):
    class B(object):
        def __getitem__(self, b):
            return (b-2)*2**(b-2)
    bmax = int(math.ceil(math.log(eps*n)/math.log(2.)))
    b = bisect.bisect(B(), eps*n, 2, bmax+2)-1
    k = int(math.ceil(float(n)/2**(b-1)))
    
    return b,k

class Quantiler(object):
    def __init__(self, n, eps=0.01):
        self.n = n
        self.len = 0
        self.bufindex = {}
        self.bufs = set()
        self.b,self.k = compute_bk(n,eps)
        self.partial = []
        self.min = self.max = None

    def append(self, x):
        self.min = min(self.min, x)
        self.max = max(self.max, x)
        self.len += 1
        self.partial.append(x)

        if self.len == self.n:
            # we're done, pad last buffer with dummy values
            flag = False
            while len(self.partial) < self.k:
                self.len += 1
                self.partial.append(self.max if flag else self.min)
                flag = not flag

        if len(self.partial) == self.k:
            # buffer is full, add to bufs
            if len(self.bufs) == self.b:
                # self.bufs is full, collapse two buffers with same weight
                for (weight, bufs) in self.bufindex.iteritems():
                    if len(bufs) >= 2:
                        buf1, buf2 = bufs[:2]
                        self.bufs.remove(buf1)
                        self.bufs.remove(buf2)
                        del bufs[:2]
                        buf = collapse([buf1,buf2])
                        self.bufs.add(buf)
                        self.bufindex.setdefault(buf.weight,[]).append(buf)
                        break
                else:
                    raise ThisShouldNotHappen("couldn't find two buffers to collapse")

            buf = Buffer(self.partial,1)
            self.bufs.add(buf)
            self.bufindex.setdefault(buf.weight,[]).append(buf)
            self.partial = []

        if self.len >= self.n:
            # we're done, build the final summary
            xs = [(x,buf.weight) for buf in self.bufs for x in buf.xs]
            xs.sort()
            self.summary_i = []
            self.summary_x = []

            i = 0
            for (x,xweight) in xs:
                self.summary_i.append(i)
                self.summary_x.append(x)
                i += xweight


    def __getitem__(self, phi):
        i = bisect.bisect(self.summary_i, int(math.ceil(phi*self.len)))-1
        return self.summary_x[i]

if __name__ == "__main__":
    import optparse, fileinput
    parser = optparse.OptionParser()
    parser.add_option("-n", dest="n_elements", help="number of elements", type=int)
    parser.add_option("-q", dest="n_quantiles", help="number of quantiles", type=int)
    parser.add_option("-m", dest="means", action="store_true", help="output medians instead of boundaries")
    opts, args = parser.parse_args()
    
    q = Quantiler(opts.n_elements)

    n_found = 0
    for line in fileinput.input(args):
        q.append(float(line))
        n_found += 1
        if n_found >= opts.n_elements:
            break
    if n_found < opts.n_elements:
        sys.stderr.write("warning: %d elements promised, only %d elements given\n" % (opts.n_elements, n_found))

    if opts.means:
        for i in xrange(opts.n_quantiles):
            phi = (i+0.5)/opts.n_quantiles
            print q[phi]
    else:
        for i in xrange(opts.n_quantiles):
            phi = float(i)/opts.n_quantiles
            print q[phi]
        print q[1.0]
