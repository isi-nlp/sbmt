#!/usr/bin/env python

import sys, os.path
import random, itertools
from mpi4py import MPI

import monitor, log
import svector
import oracle

# Which features to show in log file
watch_features = svector.Vector("lm1=1 lm2=1 pef=1 pfe=1 lef=1 lfe=1 word=1 green=1 unknown=1")

def thereader(infile, reffiles):
    if opts.input_lattice:
        insents = decoder.read_lattices(infile)
    else:
        insents = decoder.read_raw(infile)

    insents = zip_refs(insents, reffiles)
    return insents

def zip_refs(infile, reffiles):
    for (sent, refs) in itertools.izip(infile, itertools.izip(*reffiles)):
        sent.refs = [ref.split() for ref in refs]
        yield sent

def pmap(comm, nodes, inputs, f=None, g=None):
    """f gets called before sending input out to slave
       g gets called after receiving output back from slave"""
    busy = {}
    idle = set(nodes)
    next = 0
    finished = 0
    outputs = [None] * len(inputs)

    while finished < len(inputs):
        while next < len(inputs) and len(idle) > 0:
            x = inputs[next]
            node = idle.pop()
            if f:
                x = f(x, node=node)
            comm.send(x, dest=node)
            busy[node] = next
            next += 1

        status = MPI.Status()
        y = comm.recv(source=MPI.ANY_SOURCE, status=status)
        node = status.source
        if node not in busy:
            log.writeln("warning: received output from idle node %s" % node)
        if g:
            y = g(y, node=node)
        outputs[busy[node]] = y
        del busy[node]
        idle.add(node)
        finished += 1
    return outputs

if __name__ == "__main__":
    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option("-W", dest="outweightfilename", help="filename to write weights to")
    optparser.add_option("-L", dest="outscorefilename", help="filename to write BLEU scores to")
    optparser.add_option("-B", dest="bleuvariant", default="NIST")
    optparser.add_option("-S", dest="stopfile")
    optparser.add_option("-p", dest="parallel", type=int, help="how many slaves to start", default=1)
    optparser.add_option("--input-lattice", dest="input_lattice", action="store_true")
    optparser.add_option("--holdout", "--heldout-ratio", dest="heldout_ratio", help="fraction of sentences to hold out", type=float, default=None)
    optparser.add_option("--heldout-sents", dest="heldout_sents", help="number of sentences to hold out", type=int, default=None)
    optparser.add_option("--heldout-policy", dest="heldout_policy", default="last", help="which sentences to hold out (first, last, uniform)")
    optparser.add_option("--no-shuffle", dest="shuffle_sentences", action="store_false", default=True)

    try:
        configfilename = sys.argv[1]
    except IndexError:
        sys.stderr.write("usage: trainer_master.py config-file source-file reference-files [options...]\n")
        sys.exit(1)

    if log.level >= 1:
        log.write("Reading configuration from %s\n" % configfilename)
    execfile(configfilename)

    opts, args = optparser.parse_args(args=sys.argv[2:])

    comm = MPI.COMM_SELF.Spawn(sys.executable, 
                               args=[os.path.join(sys.path[0], 'trainer_slave.py')] + sys.argv[1:],
                               maxprocs=opts.parallel)
    log.prefix = "[M] "
    slaves = frozenset(range(comm.Get_remote_size()))

    # Prepare training data

    infile = file(args[0])
    log.write("source:     %s\n" % infile)

    reffilenames = args[1:]
    log.write("references: %s\n" % " ".join(reffilenames))

    insents = list(thereader(infile, [file(fn) for fn in reffilenames]))

    # Separate heldout data
    if opts.heldout_ratio is not None:
        n_heldout = int(opts.heldout_ratio * len(insents))
    elif opts.heldout_sents is not None:
        n_heldout = opts.heldout_sents
    else:
        n_heldout = 0

    if opts.heldout_policy == "first":
        heldoutsents = insents[:n_heldout]
        trainsents = insents[n_heldout:]
    elif opts.heldout_policy == "last":
        n_train = len(insents) - n_heldout
        trainsents = insents[:n_train]
        heldoutsents = insents[n_train:]
    elif opts.heldout_policy == "uniform":
        m = len(insents) / n_heldout
        trainsents = []
        heldoutsents = []
        for i, sent in enumerate(insents):
            if i % m == 0:
                heldoutsents.append(sent)
            else:
                trainsents.append(sent)
    else:
        sys.stderr.write("invalid heldout-policy %s\n" % opts.heldout_policy)
        sys.exit(1)

    # Prepare output files
    if opts.outweightfilename:
        outweightfile = open(opts.outweightfilename, "w")
    if opts.outscorefilename:
        outscorefile = open(opts.outscorefilename, "w")

    theoracle = oracle.Oracle(order=4, variant=opts.bleuvariant, oracledoc_size=10)

    requests = []

    for epoch in itertools.count(start=1):
        log.writeln("epoch %d" % epoch)

        # Process training data

        if opts.shuffle_sentences:
            random.shuffle(trainsents)

        def f(sent, node):
            log.writeln("sentence %s -> node %s" % (sent.id, node))
            return ('train', sent)
        def g(sent, node):
            log.writeln("node %s -> sentence %s" % (node, sent.id))

            # Flush out filled requests
            filled = [i for i in xrange(len(requests)) if requests[i].Test()] # test() seems buggy, why?
            for i in reversed(filled):
                requests[i:i+1] = []

            # Send update to other slaves
            for othernode in slaves:
                if othernode == node: continue
                log.writeln("update for sentence %s -> node %s" % (sent.id, othernode))
                requests.append(comm.isend(('update', sent), dest=othernode, tag=1))
            return sent

        outsents = pmap(comm, slaves, trainsents, f, g)

        # Score outputs

        train_score_comps = svector.Vector()
        for sent in outsents:
            if sent:
                train_score_comps += sent.score_comps
        train_bleu = theoracle.make_weights().dot(train_score_comps)

        # Average weights over iterations and nodes

        for node in slaves:
            comm.send(('gather-weights',), dest=node)

        all_sum_weights = comm.gather(None, root=MPI.ROOT)
        all_n_weights = comm.gather(None, root=MPI.ROOT)
        log.writeln("receive summed weights from slaves")
        sum_weights = sum(all_sum_weights, svector.Vector())
        outweights = sum_weights/float(sum(all_n_weights))

        for node in slaves:
            comm.send(('push-weights', outweights), dest=node)
        log.write("averaged feature weights: %s\n" % (outweights * watch_features))
        if opts.outweightfilename:
            outweightfile.write("%s\n" % outweights)
            outweightfile.flush()
            
        # Process heldout data

        def f(sent, node):
            log.writeln("sentence %s -> node %s" % (sent.id, node))
            return ('translate', sent)
        def g(sent, node):
            log.writeln("node %s -> sentence %s" % (node, sent.id))
            return sent

        outsents = pmap(comm, slaves, heldoutsents, f, g)

        for node in slaves:
            comm.send(('pop-weights',), dest=node)

        # Score outputs

        heldout_score_comps = svector.Vector()
        for outsent in outsents:
            if outsent:
                heldout_score_comps += outsent.score_comps
        heldout_bleu = theoracle.make_weights().dot(heldout_score_comps)

        log.write("Done: epoch=%s heldout-BLEU=%s train-BLEU=%s\n" % (epoch, heldout_bleu, train_bleu))
        if opts.outscorefilename:
            outscorefile.write("heldout=%s train=%s\n" % (heldout_bleu, train_bleu))
            outscorefile.flush()

        if opts.stopfile and os.path.exists(opts.stopfile):
            log.write("stop file %s detected, exiting\n" % opts.stopfile)
            break

    for node in slaves:
        comm.send(('end',), dest=node)
