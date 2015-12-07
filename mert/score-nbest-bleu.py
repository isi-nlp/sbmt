#!/auto/hpc-22/dmarcu/nlg/blobs/python-2.4/bin/python2.4

# Copyright (c) 2005 University of Maryland.

"""Read in n-best list in mert.perl format and replace translations with
BLEU component scores."""

"""usage: score-nbest.py <reffile>+"""

import sys, itertools
import bleu

if __name__ == "__main__":
    try:
        import psyco
        psyco.full()
    except ImportError:
        pass

    import getopt
    (opts,args) = getopt.getopt(sys.argv[1:], "cr:", [])
    for (opt,parm) in opts:
        if opt == "-c":
            bleu.preserve_case = True
        elif opt == "-r":
            bleu.eff_ref_len = parm

    if len(args) < 1:
        sys.stderr.write("must supply at least one reference set")
        raise ValueError
    
    cookedrefs = []
    reffiles = [file(name) for name in args]
    for refs in itertools.izip(*reffiles):
        cookedrefs.append(bleu.cook_refs(refs))
    
    cur_sentnum = None
    testsents = set()
    progress = 0

    infile = sys.stdin

    line = infile.readline()
    while line != "":
        try:
            (sentnum, sent, vector) = line.split('|||')
        except:
            sys.stderr.write("ERROR: bad input line: %s\n" % line)
            continue
        sentnum = int(sentnum)
        sent = " ".join(sent.split())
        vector = vector.strip()
        if False and sent == "":
            progress += 1
            line = infile.readline()
            continue
        comps = bleu.cook_test(sent, cookedrefs[sentnum])
        if comps['testlen'] != comps['guess'][0]:
            sys.stderr.write("ERROR: test length != guessed 1-grams\n")
        sys.stdout.write("%d ||| %s %d ||| %s\n" % (sentnum,
                                                
                                                   " ".join(["%d %d" % (c,g) for (c,g) in zip(comps['correct'], comps['guess'])]),
                                                   comps['reflen'],
                                                   
                                                   vector))

        sys.stdout.flush()

        if sentnum != cur_sentnum:
            sys.stderr.write(".")
            sys.stderr.flush()
            cur_sentnum = sentnum

        progress += 1
        line = infile.readline()

    sys.stderr.write("\n")

        
            
    

