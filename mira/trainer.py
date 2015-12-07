#!/usr/bin/env python

import sys, os.path, math, gc, time
import collections, random, itertools

import decoder
import model
import monitor, log
import svector
import oracle
import maxmargin

### Per-feature learning rates
# possible values are: "arow", "gauss-newton", "adagrad"
update_feature_scales = "gauss-newton"

# The following two parameters set the initial feature learning rates.
# If a feature is in initial_feature_learning_rates, its value there
# is its initial learning rate. Otherwise, it is initial_learning_rate.
initial_learning_rate = 1.
initial_feature_learning_rates = svector.Vector()

# The maximum learning rate for any feature (or None)
max_learning_rate = 0.1

# The following only applies to arow:
# How quickly to slow down per-feature learning rates.
learning_rate_decay = 0.01

# The following only applies to gauss-newton/adagrad:
# The learning rate for a feature with unit variance.
unit_learning_rate = 0.01
initial_learning_rate_strength = 1

# The above parameters get used here and only here:
def compute_feature_learning_rate(f):
    if f in initial_feature_learning_rates:
        r0 = initial_feature_learning_rates[f]
    else:
        r0 = initial_learning_rate
    if update_feature_scales == "arow":
        # \Sigma^{-1} := \Sigma^{-1} + learning_rate_decay * sum_updates2[f]
        variance = 1./r0 + sum_updates2[f] * learning_rate_decay
        r = 1. / variance
    elif update_feature_scales == "gauss-newton":
        variance = (initial_learning_rate_strength/r0 + sum_updates2[f] / unit_learning_rate) / (initial_learning_rate_strength + n_updates)
        r = 1. / variance
    elif update_feature_scales == "adagrad":
        # bug: shouldn't divide by n_updates
        variance = (initial_learning_rate_strength/r0**2 + sum_updates2[f] / unit_learning_rate**2) / (initial_learning_rate_strength + n_updates)
        r = 1. / variance**0.5
    if max_learning_rate:
        r = min(max_learning_rate, r)
    return r

# Weight on BLEU score used to compute hope translations.
hope_weight = 1.
# Which features to show in log file
watch_features = svector.Vector("lm1=1 lm2=1 pef=1 pfe=1 lef=1 lfe=1 word=1 green=1 unknown=1")

online_learning = True
cache_hyps = False
shuffle_sentences = True
loop_forever = True

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

class ForestInstance(maxmargin.Instance):
    def __init__(self, sentid, goal):
        maxmargin.Instance.__init__(self, instance_id=sentid)
        self.sentid = sentid
        self.goal = goal

    def get_fear(self):
        """Assumes that oraclemodel.input() has been called"""
        if not self.goal:
            raise NotImplementedError()
        weights = theoracle.make_weights(additive="edge")
        # use in-place operations because theoracle.make_weights might
        # be a subclass of svector.Vector
        weights += self.qp.mweights
        self.goal.reweight(weights)

        fear_vector, fear = decoder.get_nbest(self.goal, 1, 1)[0]
        fear_ovector = theoracle.finish(fear_vector, fear)
        fear_mvector = theoracle.clean(fear_vector)

        if log.level >= 1:
            log.write("fear hyp: %s\n" % " ".join(sym.tostring(e) for e in fear))
            log.write("fear features: %s\n" % fear_mvector)
            log.write("fear oracle: %s\n" % fear_ovector)

        return maxmargin.Hypothesis(fear_mvector, fear_ovector)

    def get_hope(self):
        """Assumes that oraclemodel.input() has been called"""
        if not self.goal:
            _, hope = min((self.qp.mweights.dot(hyp.mvector) + hope_weight * self.qp.oweights.dot(hyp.ovector), hyp) for hyp in self.hyps)
            return hope
            
        weights = theoracle.make_weights(additive="edge")
        # use in-place operations because theoracle.make_weights might
        # be a subclass of svector.Vector
        weights *= -hope_weight
        weights += self.qp.mweights
        self.goal.reweight(weights)

        hope_vector, hope = decoder.get_nbest(self.goal, 1, 1)[0]
        hope_ovector = theoracle.finish(hope_vector, hope)
        hope_mvector = theoracle.clean(hope_vector)

        if log.level >= 1:
            log.write("hope hyp: %s\n" % " ".join(sym.tostring(e) for e in hope))
            log.write("hope features: %s\n" % hope_mvector)
            log.write("hope oracle: %s\n" % hope_ovector)
        return maxmargin.Hypothesis(hope_mvector, hope_ovector)

def make_oracle():
    return oracle.Oracle(order=4, variant=opts.bleuvariant, oracledoc_size=10)

if __name__ == "__main__":
    gc.set_threshold(100000,10,10)
    
    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option("-W", dest="outweightfilename", help="filename to write weights to")
    optparser.add_option("-L", dest="outscorefilename", help="filename to write BLEU scores to")
    optparser.add_option("-B", dest="bleuvariant", default="NIST")
    optparser.add_option("-S", dest="stopfile")
    optparser.add_option("--input-lattice", dest="input_lattice", action="store_true")
    optparser.add_option("--holdout", "--heldout-ratio", dest="heldout_ratio", help="fraction of sentences to hold out", type=float, default=None)
    optparser.add_option("--heldout-sents", dest="heldout_sents", help="number of sentences to hold out", type=int, default=None)
    optparser.add_option("--heldout-policy", dest="heldout_policy", default="last", help="which sentences to hold out (first, last, uniform)")
    optparser.add_option("--no-shuffle", dest="shuffle_sentences", action="store_false", default=True)

    try:
        configfilename = sys.argv[1]
    except IndexError:
        sys.stderr.write("usage: trainer.py config-file source-file reference-files [options...]\n")
        sys.exit(1)

    if log.level >= 1:
        log.write("Reading configuration from %s\n" % configfilename)
    execfile(configfilename)

    opts, args = optparser.parse_args(args=sys.argv[2:])

    shuffle_sentences = opts.shuffle_sentences

    opts.parallel = True # fix me

    import parallel
    from mpi4py import MPI

    if not opts.parallel or parallel.rank == parallel.master:
        infile = file(args[0])
        log.write("source:     %s\n" % infile)

        reffilenames = args[1:]
        log.write("references: %s\n" % " ".join(reffilenames))

        insents = list(thereader(infile, [file(fn) for fn in reffilenames]))

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
        trainsents = list(enumerate(trainsents))

        output_file = sys.stdout

        if opts.outweightfilename:
            outweightfile = open(opts.outweightfilename, "w")
        if opts.outscorefilename:
            outscorefile = open(opts.outscorefilename, "w")
    else:
        insents = trainsents = heldoutsents = [] # dummy

    sum_updates2 = svector.Vector()
    n_updates = 0

    maxmargin.watch_features = watch_features

    theoracle = make_oracle()

    if cache_hyps:
        hyp_cache = collections.defaultdict(list)

    requests = []
    sumweights_helper = svector.Vector()
    nweights = 0

    if not opts.parallel or parallel.rank != parallel.master:
        thedecoder = make_decoder()

        if log.level >= 1:
            gc.collect()
            log.write("all structures loaded, memory=%s\n" % (monitor.memory()))

        decoder_errors = 0

    def process(sent):
        # Need to add an flen attribute that gives the length of the input sentence.
        # In the lattice-decoding case, we have to make a guess.
        distance = sent.compute_distance()
        sent.flen = distance.get((0,sent.n-1), None) # could be missing if n == 0

        theoracle.input(sent)
        
        global decoder_errors
        try:
            goal = thedecoder.translate(sent)
            thedecoder.process_output(sent, goal)
            decoder_errors = 0
            if goal is None: raise Exception("parse failure")
        except Exception:
            import traceback
            log.write("decoder raised exception: %s" % "".join(traceback.format_exception(*sys.exc_info())))
            decoder_errors += 1
            if decoder_errors >= 3:
                log.write("decoder failed too many times, passing exception through!\n")
                raise
            else:
                return

        goal.rescore(theoracle.models, thedecoder.weights, add=True)
            
        best_vector, best = decoder.get_nbest(goal, 1)[0]
        best_mvector = theoracle.clean(best_vector)
        best_ovector = theoracle.finish(best_vector, best)
        best_loss = theoracle.make_weights(additive="sentence").dot(best_ovector)
        log.write("best hyp: %s %s cost=%s loss=%s\n"  % (" ".join(sym.tostring(e) for e in best), best_vector, thedecoder.weights.dot(best_mvector), best_loss))

        # Set up quadratic program
        qp = maxmargin.QuadraticProgram()
        cur_instance = ForestInstance(sent.id, goal)
        qp.add_instance(cur_instance)

        if opts.parallel:
            while MPI.COMM_WORLD.Iprobe(tag=1, source=MPI.ANY_SOURCE):
                log.writeln("received update...\n")
                recv_instance = MPI.COMM_WORLD.recv(tag=1, source=MPI.ANY_SOURCE)
                log.writeln("received update for %s" % (recv_instance.instance_id,))
                # need to check for duplicate instances?
                qp.add_instance(recv_instance)

        # Add cached hyps
        if cache_hyps:
            for instance in qp.instances:
                hyps = hyp_cache[instance.instance_id]
                if len(hyps) > 0:
                    log.writeln("retrieved %d cached hyps for %s" % (len(hyps), instance.instance_id))
                for hyp in hyps:
                    instance.add_hyp(hyp)

        # Make oracle weight vector
        oweights = theoracle.make_weights(additive="sentence")
        oweights *= -1

        # Make vector of learning rates
        # We have to be careful to assign a learning rate to every possible feature
        # This is not very efficient
        feats = set()
        for item in goal.bottomup():
            for ded in item.deds:
                feats.update(ded.dcost)
        for instance in qp.instances:
            for hyp in instance.hyps:
                feats.update(hyp.mvector)
        learning_rates = svector.Vector()
        for feat in feats:
            learning_rates[feat] = compute_feature_learning_rate(feat)
        if log.level >= 3:
            log.writeln("learning rate vector: %s" % learning_rates)

        qp.optimize(thedecoder.weights, oweights, learning_rate=learning_rates)

        thedecoder.weights.compact()
        log.write("feature weights: %s\n" % (thedecoder.weights * watch_features))

        # update weight sum for averaging
        global nweights, sumweights_helper

        # sumweights_helper = \sum_{i=0}^n (i \Delta w_i)
        sumweights_helper += nweights * qp.delta_mweights()
        nweights += 1

        # update feature scales
        if update_feature_scales:
            global sum_updates2, n_updates
            for instance in qp.instances:
                """u = svector.Vector(instance.hope.mvector)
                for hyp in instance.hyps:
                    u -= hyp.alpha*hyp.mvector
                sum_updates2 += u*u"""
                for hyp in instance.hyps:
                    if hyp is not instance.hope: # hyp = instance.hope is a non-update
                        u = instance.hope.mvector - hyp.mvector
                        sum_updates2 += hyp.alpha*(u*u)
                        n_updates += hyp.alpha

            #log.write("sum of squared updates: %s\n" % (" ".join("%s=%s" % (f,sum_updates2[f]) for f in watch_features)))
            log.write("feature learning rates: %s\n" % (" ".join("%s=%s" % (f,compute_feature_learning_rate(f)) for f in watch_features)))

        if opts.parallel:
            # flush out filled requests
            global requests
            requests = [request for request in requests if not request.Test()]

            # transmit updates to other nodes
            # make a plain Instance (without forest)
            # we used to designate a hope translation,
            #send_instance = maxmargin.Instance(cur_instance.hyps, hope=cur_instance.hope, instance_id=cur_instance.sentid)
            # but now are letting the other node choose.
            send_instance = maxmargin.Instance(cur_instance.hyps, instance_id=cur_instance.sentid)

            for node in parallel.slaves:
                if node != parallel.rank:
                    requests.append(MPI.COMM_WORLD.isend(send_instance, dest=node, tag=1))

        # save all hyps for next time
        if cache_hyps:
            epsilon = 0.01
            for instance in qp.instances:
                hyps = hyp_cache[instance.instance_id]
                for hyp in instance.hyps:
                    for hyp1 in hyps:
                        if (hyp.mvector-hyp1.mvector).normsquared() <= epsilon and (hyp.ovector-hyp1.ovector).normsquared() <= epsilon:
                            break
                    else:
                        if log.level >= 2:
                            log.writeln("add hyp to cache: %s" % hyp)
                        hyps.append(hyp)

        theoracle.update(best_ovector)
        sent.score_comps = best_ovector

        if log.level >= 1:
            gc.collect()
            log.write("done updating, memory = %s\n" % monitor.memory())

        sent.ewords = [sym.tostring(e) for e in best]

        return sent

    def process_heldout(sent):
        # Need to add an flen attribute that gives the length of the input sentence.
        # In the lattice-decoding case, we have to make a guess.
        distance = sent.compute_distance()
        sent.flen = distance.get((0,sent.n-1), None) # could be missing if n == 0

        theoracle.input(sent)
        
        log.write("done preparing\n")

        global decoder_errors
        try:
            goal = thedecoder.translate(sent)
            thedecoder.process_output(sent, goal)
            decoder_errors = 0
            if goal is None: raise Exception("parse failure")
        except Exception:
            import traceback
            log.write("decoder raised exception: %s" % "".join(traceback.format_exception(*sys.exc_info())))
            decoder_errors += 1
            if decoder_errors >= 100:
                log.write("decoder failed too many times, passing exception through!\n")
                raise
            else:
                return

        goal.rescore(theoracle.models, thedecoder.weights, add=True)
            
        bestv, best = decoder.get_nbest(goal, 1)[0]
        log.write("done decoding\n")

        bestg = theoracle.finish(bestv, best)
        #bestscore = theoracle.make_weights(additive="sentence").dot(bestg)
        bestscore = theoracle.make_weights(additive="edge").dot(bestg)
        log.write("best hyp: %s %s cost=%s score=%s\n"  % (" ".join(sym.tostring(e) for e in best), bestv, thedecoder.weights.dot(bestv), bestscore))

        sent.score_comps = bestg
        sent.ewords = [sym.tostring(e) for e in best]

        return sent

    epoch = 1

    if loop_forever:
        iterations = itertools.count()
    else:
        iterations = xrange(1)

    for iteration in iterations:
        log.writeln("epoch %d" % iteration)

        # Process training data

        if shuffle_sentences and (not opts.parallel or parallel.rank == parallel.master):
            random.shuffle(trainsents)

        if opts.parallel:
            outsents = parallel.pmap(lambda (si, sent): (si, process(sent)), 
                                     trainsents, 
                                     tag=0, verbose=1)
            if parallel.rank == parallel.master:
                outsents = list(outsents)
        else:
            outsents = [(si, process(sent)) for (si,sent) in trainsents]

        if not opts.parallel or parallel.rank == parallel.master:
            outsents.sort()
            train_score_comps = svector.Vector()
            for _, outsent in outsents:
                if outsent:
                    output_file.write("%s\n" % " ".join(outsent.ewords))
                    train_score_comps += outsent.score_comps
                else:
                    output_file.write("\n") # dummy output for decoder failure

        if not opts.parallel or parallel.rank != parallel.master:
            if log.level >= 3:
                log.write("feature weights: %s\n" % thedecoder.weights)
                if update_feature_scales:
                    log.write("feature learning rates: %s\n" % " ".join("%s=%s" % (f,compute_feature_learning_rate(f)) for (f,v) in thedecoder.weights.iteritems()))
            elif log.level >= 1:
                log.write("feature weights: %s\n" % (thedecoder.weights * watch_features))
                if update_feature_scales:
                    log.write("feature learning rates: %s\n" % " ".join("%s=%s" % (f,compute_feature_learning_rate(f)) for (f,v) in (thedecoder.weights * watch_features).iteritems()))

        # average weights over iterations and nodes
        outweights = svector.Vector()
        if not opts.parallel or parallel.rank != parallel.master:
            if online_learning:
                outweights = float(nweights) * thedecoder.weights - sumweights_helper

            else:
                outweights = thedecoder.weights
                nweights = 1

            outweights.compact()

            log.write("summed feature weights: %s n=%d\n" % (outweights * watch_features, nweights))

        if opts.parallel:
            all_outweights = MPI.COMM_WORLD.gather(outweights, root=parallel.master)
            all_nweights = MPI.COMM_WORLD.gather(nweights, root=parallel.master)
            if parallel.rank == parallel.master:
                sumweights = sum(all_outweights, svector.Vector())
                outweights = sumweights/float(sum(all_nweights))
                log.write("summed feature weights: %s n=%d\n" % (sumweights * watch_features, sum(all_nweights)))
                log.write("averaged feature weights: %s\n" % (outweights * watch_features))
        else:
            outweights /= nweights

        if opts.outweightfilename:
            if not opts.parallel or parallel.rank == parallel.master:
                outweightfile.write("%s\n" % outweights)
                outweightfile.flush()

        if opts.parallel:
            outweights = MPI.COMM_WORLD.bcast(outweights, root=parallel.master)
            
        # Process heldout data

        if not opts.parallel or parallel.rank != parallel.master:
            saveweights = thedecoder.weights
            thedecoder.weights = outweights
        
        if opts.parallel:
            outsents = parallel.pmap(process_heldout, heldoutsents, tag=0, verbose=1)
        else:
            outsents = (process_heldout(sent) for sent in heldoutsents)

        if not opts.parallel or parallel.rank == parallel.master:
            heldout_score_comps = svector.Vector()
            for outsent in outsents:
                if outsent:
                    output_file.write("%s\n" % " ".join(outsent.ewords))
                    heldout_score_comps += outsent.score_comps
                else:
                    output_file.write("\n") # dummy output for decoder failure
                output_file.flush()

        if not opts.parallel or parallel.rank == parallel.master:
            train_bleu = theoracle.make_weights().dot(train_score_comps)
            heldout_bleu = theoracle.make_weights().dot(heldout_score_comps)
            log.write("Done: epoch=%s heldout-BLEU=%s train-BLEU=%s\n" % (epoch, heldout_bleu, train_bleu))
            if opts.outscorefilename:
                outscorefile.write("heldout=%s train=%s\n" % (heldout_bleu, train_bleu))
                outscorefile.flush()

        if not opts.parallel or parallel.rank != parallel.master:
            thedecoder.weights = saveweights

        epoch += 1

        if opts.stopfile and os.path.exists(opts.stopfile):
            log.write("stop file %s detected, exiting\n")
            break
