import sys
import forest
import re
from optparse import OptionParser

optparser = OptionParser()
optparser.add_option("-e","--exhaustive", action="store_true", default="False")

def main(forest_file,nbest_file,weight_string,exhaustive=False):
    exhaustive=False
    ffile = open(forest_file,"r")
    nfile = open(nbest_file,"r")
    weights = forest.read_features(weight_string)

    expr = re.compile('.*sent=(\d+).*nbest=(\d+).*totalcost=(\S+).*derivation={{{(.*?)}}}.*')
    
    sent = -1
    count = 0
    kbest = 0
    f = forest.Forest([])
    passed = True
    for line in nfile:        
        m = re.match(expr,line)
        count = int(m.group(2))
        if sent != int(m.group(1)):
            sent = int(m.group(1))
            f = forest.read_forest(ffile.readline())
            if exhaustive:
                kbest = forest.kbest(f,weights)
        deriv = forest.read_tree(m.group(4))
        cost = float(m.group(3))
        if exhaustive:
            fderiv = kbest.next()
        else:
            fintersect = forest.kbest(forest.intersect_forest_tree(f,deriv),weights)
            fderiv = fintersect.next()
            empty = False
            try:
                fffff = fintersect.next()
            except StopIteration:
                empty = True
            assert(empty)
        s = fderiv.score #forest.inner_prod(fderiv.features,weights)
        
        if abs(s - cost) > 0.01:
            print "==="
            print "%s: %s vs %s" % (count, s,cost) 
            print deriv
            print fderiv
            print line
            print "==="
            passed = False
        count += 1
    if passed:
        print "passed"
    else:
        print "failed"

if __name__ == "__main__":
    opts,args = optparser.parse_args(sys.argv[1:])
    main(*args,**opts.__dict__)