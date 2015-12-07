#!/usr/bin/env python

import sys, os.path, math, gc, time
import collections, random, itertools
import sym
import decoder
import model
import monitor, log
import svector
import oracle

import sgml

def remove_zeros(vec):
    zeros = []
    for (f,v) in vec.iteritems():
        if abs(v) == 0.:
            zeros.append(f)
    for f in zeros:
        del vec[f]

def thereader(infile, reffiles):
    for (sent, refs) in itertools.izip(sgml.read_raw(infile), itertools.izip(*reffiles)):
        sent.refs = [ref.split() for ref in refs]
        yield sent

update_feature_scales = False
def compute_variance(ss, n):
    return max(1.,ss/n*max_learning_rate/unit_learning_rate)

if update_feature_scales:
    # The (maximum) learning rate for a feature with unit variance.
    unit_learning_rate = 0.05
    # The (maximum) maximum learning rate for any feature
    max_learning_rate = 0.1
else:
    max_learning_rate = 0.01

l1_regularization = None
#l1_regularization = 0.001
l2_regularization = None
#l2_regularization = 0.001

online_learning = True
shuffle_sentences = True
loop_forever = True

hope_weight = 1.
fear_weight = 1.

watch_features = svector.Vector("lm1=1 lm2=1 pef=1 pfe=1 lef=1 lfe=1 word=1")

n_best = 1

def normsquared(v):
    """calculates squared norm of v using feature_scales"""
    result = 0.
    for feat,val in v.iteritems():
        result += val**2*feature_scales[feat]
    return result

def apply_update(weights, update):
    for feat,val in update.iteritems():
        weights[feat] -= update[feat]*feature_scales[feat]
    return weights

def get_gold(sent, goal, weights):
    """Assumes that oraclemodel.input() has been called"""
    oracleweights = theoracle.make_weights(additive=True)
    # we use the in-place operations because oracleweights might be
    # a subclass of Vector
    oracleweights *= -hope_weight
    oracleweights += weights

    goal.reweight(oracleweights)

    goldv, gold = decoder.get_nbest(goal, 1, 1)[0]
    goldscore = get_score(goldv, gold)

    log.write("gold hyp: %s %s cost=%s score=%s\n"  % (" ".join(sym.tostring(e) for e in gold), goldv, weights.dot(goldv), goldscore))

    # the learner MUST not see the oracle features
    goldv = theoracle.clean(goldv)

    return goldv, gold, goldscore

def get_hyps(sent, goal, weights):
    """Assumes that oraclemodel.input() has been called"""
    # worst violators

    oracleweights = theoracle.make_weights(additive=True)
    # we use the in-place operations because oracleweights might be
    # a subclass of Vector
    oracleweights *= fear_weight
    oracleweights += weights

    goal.reweight(oracleweights)

    hyps = decoder.get_nbest(goal, 1, 1)
    result = []

    for hypv, hyp in hyps:
        hypscore = get_score(hypv, hyp)
        log.write("added new hyp: %s %s cost=%s score=%s\n"  % (" ".join(sym.tostring(e) for e in hyp), hypv, weights.dot(hypv), hypscore))

        # the learner MUST not see the oracle features
        hypv = theoracle.clean(hypv)

        result.append((hypv, hyp, hypscore))

    return result

def get_score(hypv, hyp):
    hypv = theoracle.finish(hypv, hyp)
    return theoracle.make_weights(additive=True).dot(hypv)

### Weight update algorithms

class StopOptimization(Exception):
    pass

def cutting_plane(weights, updates, alphas, oracles={}, epsilon=0.01):
    done = False
    saveweights = svector.Vector(weights)

    if l2_regularization:
        # not using feature scales
        #weights *= 1./(1+len(updates)*l2_regularization*max_learning_rate)

        # using feature scales
        for f in weights:
            weights[f] *= 1./(1.+feature_scales[f]*len(updates)*l2_regularization*max_learning_rate)

    while not done:
        # call separation oracles
        done = True
        for sentid, oracle in oracles.iteritems():
            vscores = oracle(weights)
            for v, score in vscores:
                violation = weights.dot(v) + score

                for v1, score1 in updates[sentid]:
                    violation1 = weights.dot(v1) + score1
                    if violation <= violation1 + epsilon:
                        break
                else:
                    updates[sentid].append((v, score))
                    alphas[sentid].append(0.)
                    done = False

        weights, alphas = update_weights(weights, updates, alphas)

        if log.level >= 4:
            log.write("alphas: %s\n" % alphas)

    if False and log.level >= 1:
        log.write("weight update: %s\n" % " ".join("%s=%s" % (f,v) for f,v in (weights-saveweights).iteritems() if abs(v) > 0.))

    return weights, alphas

def update_weights(weights, updates, alphas):
    # sequential minimum optimization
    # minimize 1/2 ||sum(updates)||**2 + C*sum(xis)
    # one xi for all candidates for each sentence
    # s.t. each margin >= loss - xi
    # s.t. each xi >= 0

    # these are not sensitive to feature_scales, but maybe they should be
    # this is not right -- gammas should be preserved across calls
    if l1_regularization:
        gammas = svector.Vector()

    iterations = 0
    done = False
    while not done:
        if l1_regularization:
            for f in weights:
                delta = max(-l1_regularization*max_learning_rate*len(updates)-gammas[f], min(weights[f], l1_regularization*max_learning_rate*len(updates)-gammas[f]))
                gammas[f] += delta
                weights[f] += -delta
            if log.level >= 4:
                log.write("  gammas: %s\n" % gammas)

        done = True
        sentids = updates.keys()
        #random.shuffle(sentids)
        for sentid in sentids:
            vscores = updates[sentid]
            if len(vscores) < 2:
                continue
            if log.level >= 4:
                log.write("  sentence %s\n" % sentid)
            try:
                weights, alphas[sentid] = update_sentence_weights(weights, updates[sentid], alphas[sentid])
                done = False
            except StopOptimization:
                pass

            if log.level >= 4:
                log.write("    alphas: %s\n" % (" ".join(str(alpha) for alpha in alphas[sentid])))

        iterations += 1
        if iterations > 1000:
            log.write("  SMO: 1000 passes through data, stopping\n")
            break

        #log.write("  intermediate weights: %s\n" % weights)

    return weights, alphas

def update_sentence_weights(weights, updates, alphas=None):
    # sequential minimum optimization (bicoordinate)
    # minimize ||sum(updates)|| + C*xi
    # s.t. each margin >= loss - xi
    # s.t. xi >= 0

    # The KKT conditions are:
    # alphas[i] = 0 => weights.dot(updates[i]) + losses[i] is not the max
    # alphas[i] > 0 => weights.dot(updates[i]) + losses[i] is the max

    updates, losses = zip(*updates)

    n = len(updates)
    vs = [weights.dot(updates[i]) + losses[i] for i in xrange(n)]
    if log.level >= 4:
        log.write("    violations: %s\n" % (" ".join(str(violation) for violation in vs)))
    j,i = select_pair(vs, alphas)

    delta = compute_alpha(weights,
                          updates[j]-updates[i],
                          losses[j]-losses[i],
                          -alphas[j],
                          alphas[i])
    alphas[j] += delta
    alphas[i] -= delta

    if log.level >= 4:
        log.write("    update direction = %s\n" % (updates[j]-updates[i]))
        log.write("    moving delta=%s from %s to %s\n" % (delta, i, j))

    weights = apply_update(weights, delta*(updates[j]-updates[i]))

    return weights, alphas

def select_pair(vs, alphas, epsilon=0.01):
    n = len(vs)
    assert n >= 2
    for i in xrange(n):
        vs_max = max(vs[j] for j in xrange(n) if j != i)

        if alphas[i] == 0 and vs[i] > vs_max + epsilon:
            # i is the worst violator but has no weight yet
            # find someone to take weight from
            for j in xrange(n):
                if j != i and alphas[j] > 0:
                    return i,j

        if alphas[i] > 0 and vs[i] < vs_max - epsilon:
            # j has weight but is not the worst violator
            # find a worse violator to give weight to
            for j in xrange(n):
                if j != i and vs[j] > vs[i] + epsilon:
                    return j,i

    raise StopOptimization()

def compute_alpha(weights, update, loss, minalpha, maxalpha):
    """MIRA formula for update size"""
    sumsq = normsquared(update)
    margin = -weights.dot(update)
    if log.level >= 4:
        log.writeln("delta = (%s-%s)/%s" % (loss, margin, sumsq))
    if sumsq > 0.:
        alpha = (loss-margin)/sumsq
        alpha = max(minalpha, min(maxalpha, alpha))
    elif loss-margin > 0.:
        alpha = maxalpha
    elif loss-margin < 0.:
        alpha = minalpha
    else:
        log.write("compute_alpha: 0/0, this shouldn't happen\n")
        alpha = 0.

    return alpha

def make_oracle():
    return oracle.Oracle(order=4, variant=opts.bleuvariant, oracledoc_size=10)

if __name__ == "__main__":
    gc.set_threshold(100000,10,10)

    import optparse
    optparser = optparse.OptionParser()
    optparser.add_option("-p", "--parallel", dest="parallel", help="parallelize using MPI", action="store_true", default=False)
    optparser.add_option("-W", dest="outweightfilename", help="filename to write weights to")
    optparser.add_option("-B", dest="bleuvariant", default="NIST")
    optparser.add_option("-S", dest="stopfile")
    optparser.add_option("--holdout", "--heldout-ratio", dest="heldout_ratio", help="fraction of sentences to hold out", type=float, default=None)
    optparser.add_option("--heldout-sents", dest="heldout_sents", help="number of sentences to hold out", type=int, default=None)
    optparser.add_option("--no-shuffle", dest="shuffle_sentences", action="store_false", default=True)

    try:
        configfilename = sys.argv[1]
    except IndexError:
        sys.stderr.write("usage: train.py config-file source-file reference-files [options...]\n")
        sys.exit(1)

    log.writeln("Starting at %s"%log.datetoday())
    if log.level >= 1:
        log.write("Reading configuration from %s\n" % configfilename)
    execfile(configfilename)

    opts, args = optparser.parse_args(args=sys.argv[2:])

    shuffle_sentences = opts.shuffle_sentences

    if opts.parallel:
        import parallel
        import mpi
        log.prefix="[%s] " % parallel.rank

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
        n_train = len(insents) - n_heldout
        trainsents = list(enumerate(insents[:n_train]))
        heldoutsents = insents[n_train:]
        log.write("%d training sentences\n" % n_train)
        log.write("%d heldout sentences\n" % (len(insents)-n_train))

        output_file = sys.stdout

        if opts.outweightfilename:
            outweightfile = open(opts.outweightfilename, "w")
    else:
        insents = trainsents = heldoutsents = [] # dummy

    # Initialize feature scales
    if update_feature_scales:
        feature_scales = collections.defaultdict(lambda: 1./compute_variance(0, 1))
        sum_updates2 = svector.Vector()
        n_updates = 0
    else:
        feature_scales = collections.defaultdict(lambda: 1.)

    theoracle = make_oracle()

    requests = []
    sumweights_helper = svector.Vector()
    nweights = 0

    if not opts.parallel or parallel.rank != parallel.master:
        thedecoder = make_decoder()

        if log.level >= 1:
            gc.collect()
            log.write("all structures loaded, memory=%s\n" % (monitor.memory()))

        decoder_errors = 0

    updates = collections.defaultdict(list)
    alphas = collections.defaultdict(list)

    def process(sent):
        global alphas

        if online_learning:
            updates.clear()
            alphas.clear()

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
            log.writeln("decoder raised exception: %s %s" % (sent, "".join(traceback.format_exception(*sys.exc_info()))))
            decoder_errors += 1
            if decoder_errors >= 100:
                log.write("decoder failed too many times, passing exception through!\n")
                raise
            else:
                return

        goal.rescore(theoracle.models, thedecoder.weights, add=True)

        bestv, best = decoder.get_nbest(goal, 1)[0]
        log.write("done decoding\n")

        bestscore = get_score(bestv, best)
        log.write("best hyp: %s %s cost=%s score=%s\n"  % (" ".join(sym.tostring(e) for e in best), bestv, thedecoder.weights.dot(bestv), bestscore))

        goldv, gold, goldscore = get_gold(sent, goal, thedecoder.weights)

        assert(sent.id not in updates) # in batch learning, this can happen, and we would have to undo the update associated with this sentence

        updates[sent.id] = [(svector.Vector(), 0.)]
        alphas[sent.id] = [max_learning_rate]

        if opts.parallel:
            while True:
                if mpi.world.iprobe(tag=1):
                    (sentid, vscores) = mpi.world.recv(tag=1)
                    log.write("received update for %s\n" % (sentid,))

                    if sentid in updates: # see comment above
                        log.write("ignoring update for %s\n" % (sentid,))
                        continue # drop this update on the floor

                    updates[sentid] = vscores
                    alphas[sentid] = [max_learning_rate] + [0.]*(len(vscores)-1)
                    # since the first update is zero, the alphas & updates
                    # are still consistent with weights
                else:
                    break

        def oracle(weights):
            hyps = get_hyps(sent, goal, weights)
            return [(goldv-hypv, goldscore-hypscore) for (hypv, hyp, hypscore) in hyps]

        thedecoder.weights, alphas = cutting_plane(thedecoder.weights, updates, alphas, {sent.id:oracle})

        remove_zeros(thedecoder.weights)
        log.write("feature weights: %s\n" % (thedecoder.weights * watch_features))
        log.write("weight norm: %s\n" % (math.sqrt(thedecoder.weights.normsquared())))

        # update weight sum for averaging
        global nweights, sumweights_helper

        # sumweights_helper = \sum_{i=0}^n (i \Delta w_i)
        for sentid in updates:
            for (v,score),alpha in itertools.izip(updates[sentid], alphas[sentid]):
                apply_update(sumweights_helper, nweights * alpha * v)
        nweights += 1

        # update feature scales
        if update_feature_scales:
            global sum_updates2, n_updates, feature_scales
            for sentid in updates:
                u = svector.Vector()
                for (v,score),alpha in itertools.izip(updates[sentid], alphas[sentid]):
                    u += alpha/max_learning_rate*v
                sum_updates2 += u*u
                n_updates += 1

            try:
                default_feature_scale = 1./compute_variance(0, n_updates)
            except ZeroDivisionError:
                default_feature_scale = 0. # pseudoinverse
            feature_scales = collections.defaultdict(lambda: default_feature_scale)
            for feat in sum_updates2:
                try:
                    feature_scales[feat] = 1./compute_variance(sum_updates2[feat], n_updates)
                except ZeroDivisionError:
                    feature_scales[feat] = 0. # pseudoinverse

            log.write("feature scales: %s\n" % (" ".join("%s=%s" % (f,feature_scales[f]) for f in watch_features if f in feature_scales)))

        if opts.parallel:
            # flush out filled requests
            global requests
            requests = [request for request in requests if not request.test()]

            # transmit updates to other nodes
            for node in parallel.slaves:
                if node != parallel.rank:
                    requests.append(mpi.world.isend(value=(sent.id, updates[sent.id]), dest=node, tag=1))

        bestv = theoracle.finish(bestv, best)
        theoracle.update(bestv)
        sent.score_comps = bestv

        if log.level >= 1:
            gc.collect()
            log.write("done updating, memory = %s\n" % monitor.memory())

        sent.ewords = [sym.tostring(e) for e in best]

        return sent

    def process_heldout(sent):
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
            log.writeln("decoder raised exception: %s %s" % (sent, "".join(traceback.format_exception(*sys.exc_info()))))
            decoder_errors += 1
            if decoder_errors >= 100:
                log.write("decoder failed too many times, passing exception through!\n")
                raise
            else:
                return

        goal.rescore(theoracle.models, thedecoder.weights, add=True)

        bestv, best = decoder.get_nbest(goal, 1)[0]
        log.write("done decoding\n")

        bestscore = get_score(bestv, best)
        log.write("best hyp: %s %s cost=%s score=%s\n"  % (" ".join(sym.tostring(e) for e in best), bestv, thedecoder.weights.dot(bestv), bestscore))

        bestv = theoracle.finish(bestv, best)
        sent.score_comps = bestv
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
                    log.write("feature scales: %s\n" % " ".join("%s=%s" % (f,v) for (f,v) in feature_scales.iteritems()))

        # average weights over iterations and nodes
        outweights = svector.Vector()
        if not opts.parallel or parallel.rank != parallel.master:
            if online_learning:
                outweights = float(nweights) * thedecoder.weights - sumweights_helper

            else:
                outweights = thedecoder.weights
                nweights = 1

            remove_zeros(outweights)

            log.write("summed feature weights: %s n=%d\n" % (outweights * watch_features, nweights))

        if opts.parallel:
            all_outweights = mpi.gather(mpi.world, outweights, parallel.master)
            all_nweights = mpi.gather(mpi.world, nweights, parallel.master)
            if parallel.rank == parallel.master:
                sumweights = sum(all_outweights, svector.Vector())
                outweights = sumweights/float(sum(all_nweights))
                log.write("summed feature weights: %s n=%d\n" % (sumweights * watch_features, sum(all_nweights)))
                log.write("averaged feature weights: %s\n" % (outweights * watch_features))

        if opts.outweightfilename:
            if not opts.parallel or parallel.rank == parallel.master:
                outweightfile.write("%s\n" % outweights)
                outweightfile.flush()

        if opts.parallel:
            outweights = mpi.broadcast(mpi.world, outweights, parallel.master)

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

        if opts.parallel:
            # Try to sync up nodes so that the log file is clear
            # for writing out the BLEU score
            #_ = mpi.gather(mpi.world, None, parallel.master)
            time.sleep(60)

        if not opts.parallel or parallel.rank == parallel.master:
            train_bleu = theoracle.make_weights().dot(train_score_comps)
            heldout_bleu = theoracle.make_weights().dot(heldout_score_comps)
            log.writeln("Done: epoch=%s heldout-BLEU=%s train-BLEU=%s %s" % (epoch, heldout_bleu, train_bleu, log.datetoday()))

        if not opts.parallel or parallel.rank != parallel.master:
            thedecoder.weights = saveweights

        epoch += 1

        if opts.stopfile and os.path.exists(opts.stopfile):
            log.write("stop file %s detected, exiting\n")
            break

