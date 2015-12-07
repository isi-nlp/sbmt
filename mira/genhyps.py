#!/usr/bin/env python

"""Stripped-down version of trainer that just outputs hypotheses that are used during
training. Will be used for error analysis purposes."""


import sys, os.path, math, gc
import collections, random, itertools

import sym
import decoder
import model
import monitor, log
import svector
import oracle
import forest

import sgml
thereader = sgml.read_raw

class Hypothesis(object):
    pass

def get_nbest(goal, n_best, ambiguity_limit=None):
    if log.level >= 1:
        log.write("  Extracting derivation(s)...\n")

    result = []

    nbest = forest.NBest(goal, ambiguity_limit=ambiguity_limit)
    for deriv in itertools.islice(nbest, n_best):
        hyp = Hypothesis()
        hyp.words = [sym.tostring(e) for e in deriv.english()]
        hyp.vector = deriv.vector()
        hyp.deriv = str(deriv)

        result.append(hyp)

    return result

def get_hyps(sent, goal, weights):
    """Assumes that oraclemodel.input() has been called"""
    hyps = []

    # model n-best
    hyps.extend(get_nbest(goal, opts.nbest, 1))

    # model+BLEU
    oracleweights = oraclemodel.make_weightvector(weights, -opts.oracleweight, oracledoc)
    goal.rescore(oraclemodels, oracleweights, add=True)
    hyps.extend(get_nbest(goal, opts.nbest, 1))

    # worst violators
    oracleweights = oraclemodel.make_weightvector(weights, +opts.oracleweight, oracledoc)
    goal.reweight(oracleweights)
    hyps.extend(get_nbest(goal, opts.nbest, 1))

    for hi,hyp in enumerate(hyps):
        hyp.n = hi
        hyp.cost = weights.dot(hyp.vector)

    return hyps

def get_score(sent):
    return (oraclemodel.srclen+oracledoc['srclen'])*oraclemodel.bleu(sent, add=oracledoc, clip=True)

oracledoc = svector.Vector("match0=1 match1=1 match2=1 match3=1 guess0=1 guess1=1 guess2=1 guess3=1 candlen=1 reflen=1 srclen=1")

def visualization_output(f, sent, hyp):
    """SBMT format."""
    f.write("NBEST sent=%s nbest=%s totalcost=%s hyp={{{%s}}} derivation={{{%s}}} %s\n" % (int(sent.id)+1, hyp.n, hyp.cost, " ".join(hyp.words), hyp.deriv, hyp.vector))

if __name__ == "__main__":
    gc.set_threshold(100000,10,10)

    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option("-p", "--parallel", dest="parallel", help="parallelize using MPI", action="store_true", default=False)
    optparser.add_option("-B", dest="bleuvariant", default="NIST")
    optparser.add_option("-O", dest="oracleweight", type=float, default=1.)
    optparser.add_option("-n", dest="nbest", type=int, default=1)

    try:
        configfilename = sys.argv[1]
    except IndexError:
        sys.stderr.write("usage: train.py config-file source-file reference-files [options...]\n")
        sys.exit(1)

    if log.level >= 1:
        log.write("Reading configuration from %s\n" % configfilename)
    execfile(configfilename)

    opts, args = optparser.parse_args(args=sys.argv[2:])

    if opts.parallel:
        import parallel
        import mpi
        log.prefix = "[%s] " % parallel.rank

    if not opts.parallel or parallel.rank == parallel.master:
        infile = file(args[0])
        log.write("source:     %s\n" % infile)

        # thereader should generate a sequence of sentence objects.
        # The sentence objects must have the following attributes:
        #   fwords: list of strings
        #   id: 0-based number in sequence, used by several models
        def refreader(reader, reffiles):
            for (sent, refs) in itertools.izip(reader, itertools.izip(*reffiles)):
                sent.refs = [ref.split() for ref in refs]
                yield sent

        reffilenames = args[1:]
        log.write("references: %s\n" % " ".join(reffilenames))

        insents = list(refreader(thereader(infile), [file(fn) for fn in reffilenames]))

        output_file = sys.stdout
    else:
        insents = [] # dummy

    oraclemodel = oracle.OracleModel(4, variant=opts.bleuvariant)

    if not opts.parallel or parallel.rank != parallel.master:
        thedecoder = make_decoder()
        oraclemodels = [oraclemodel, oracle.WordCounter()]

        if log.level >= 1:
            gc.collect()
            log.write("all structures loaded, memory=%s\n" % (monitor.memory()))

        updates = collections.defaultdict(list)
        decoder_errors = 0

    def process(sent):
        oraclemodel.input(sent)
        log.write("done preparing\n")
        try:
            goal = thedecoder.translate(sent)
        except Exception:
            import traceback
            log.writeln("decoder raised exception: %s" % "".join(traceback.format_exception(*sys.exc_info())))
            global decoder_errors
            decoder_errors += 1
            if decoder_errors >= 5:
                raise
            else:
                return
        bestv, best =decoder.get_nbest(goal, 1)[0]
        log.write("done decoding\n")

        # Collect hypotheses that will be used for learning
        sent.hyps = get_hyps(sent, goal, thedecoder.weights)
        log.write("done rescoring\n")

        return sent

    if opts.parallel:
        outsents = parallel.pmap(process, insents, tag=0, verbose=1)
    else:
        outsents = (process(sent) for sent in insents)

    if not opts.parallel or parallel.rank == parallel.master:
        bleu_comps = svector.Vector()
        for outsent in outsents:
            if outsent:
                for hyp in outsent.hyps:
                    visualization_output(output_file, outsent, hyp)
            output_file.flush()


