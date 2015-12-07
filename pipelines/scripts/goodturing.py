import sys
import collections

# discount counts up to this value
maxcount = 4
# DC: use this as a substitute for zero
#bad = 1e-6
bad = 1. # SRI-LM

def discounts(cc):
    d = collections.defaultdict(lambda: 1.)
    sys.stderr.write(" count-counts\n")
    for c in xrange(1,maxcount+2):
        sys.stderr.write("  %s %s\n" % (c, cc[c]))
    thismaxcount = maxcount
    if cc[1] == 0:
        sys.stderr.write("warning: no singletons, GT discounting disabled\n");
        return d

    while thismaxcount > 0 and cc[thismaxcount+1] == 0:
        sys.stderr.write("warning: count of count %d is zero, lowering maxcount\n" % (thismaxcount+1))
        thismaxcount -= 1

    if thismaxcount <= 1:
        # the formulas below are going to discount singletons down to zero
        # instead we apply a severe discount
        sys.stderr.write("warning: applying arbitrary discount of %s\n" % bad)
        d[1] = bad
        return d

    # this is used for renormalization:
    # we are discounting counts 1..$thismaxcount only
    # and reserving $cc->{1} counts for unseen events

    common_term = float(thismaxcount+1)*cc[thismaxcount+1]/cc[1]
    if 1.-common_term == 0:
        # this happens in rare cases, e.g., count1=2 count2=1 count3=0 ...
        sys.stderr.write("warning: 1-commonTerm is zero, GT discounting disabled\n")
        return d

    for i in xrange(1, thismaxcount+1):
        if cc[i] == 0:
            sys.stderr.write("warning: count of count %d is zero\n" % i)
            coeff = 1.
        else:
            coeff0 = float(i+1)*cc[i+1]/(i*cc[i])
            coeff = (coeff0 - common_term) / (1.-common_term)
            if coeff0 > 1.: # why coeff0?
                sys.stderr.write("warning: discount coeff %d is out of range: %f => 1\n" % (i, coeff))
                coeff = 1.
            if coeff <= 0.:
                sys.stderr.write("warning: discount coeff %d is out of range: %f => %f\n" % (i, coeff, bad))
                coeff = bad # DC

        d[i] = coeff

    sys.stderr.write(" discounts\n")
    for count in xrange(1,thismaxcount+1):
        sys.stderr.write("  %s * %s = %s\n" % (count, d[count], count*d[count]))

    return d
    
if __name__ == "__main__":
    countcount = collections.defaultdict(int)
    for line in sys.stdin:
        c, cc = line.split()
        countcount[int(c)] = int(cc)
    discounts(countcount)
    
