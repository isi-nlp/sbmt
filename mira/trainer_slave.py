#!/usr/bin/env python

import sys, os.path, math, gc, time
import collections, random, itertools
from mpi4py import MPI

import decoder
import model
import svector
import oracle
import maxmargin
import monitor, log

### Per-feature learning rates
# possible values are: "arow", "gauss-newton"
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

# The following only applies to gauss-newton:
# The learning rate for a feature with unit variance.
unit_learning_rate = 0.01
initial_learning_rate_strength = 1

# Weight on BLEU score used to compute hope translations.
hope_weight = 1.

# Which features to show in log file
watch_features = svector.Vector("lm1=1 lm2=1 pef=1 pfe=1 lef=1 lfe=1 word=1 green=1 unknown=1")

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

class Learner(object):
    def __init__(self):
        self.sum_weights_helper = svector.Vector()
        self.n_weights = 0

        self.sum_updates2 = svector.Vector()
        self.n_updates = 0

    def compute_feature_learning_rate(self, f):
        if f in initial_feature_learning_rates:
            r0 = initial_feature_learning_rates[f]
        else:
            r0 = initial_learning_rate
        if update_feature_scales == "arow":
            # \Sigma^{-1} := \Sigma^{-1} + learning_rate_decay * sum_updates2[f]
            variance = 1./r0 + sum_updates2[f] * learning_rate_decay
            r = 1. / variance
        elif update_feature_scales == "gauss-newton":
            variance = (initial_learning_rate_strength/r0 + self.sum_updates2[f] / unit_learning_rate) / (initial_learning_rate_strength + self.n_updates)
            r = 1. / variance
        if max_learning_rate:
            r = min(max_learning_rate, r)
        return r

    def train(self, sent, instances):
        # Set up quadratic program
        qp = maxmargin.QuadraticProgram()
        for instance in instances:
            qp.add_instance(instance)

        # Make oracle weight vector
        oweights = theoracle.make_weights(additive="sentence")
        oweights *= -1

        # Make vector of learning rates
        # We have to be careful to assign a learning rate to every feature in the forest
        # This is not very efficient
        feats = set()
        for instance in qp.instances:
            if hasattr(instance, "goal") and instance.goal:
                for item in instance.goal.bottomup():
                    for ded in item.deds:
                        feats.update(ded.dcost)
            for hyp in instance.hyps:
                feats.update(hyp.mvector)
        learning_rates = svector.Vector()
        for feat in feats:
            learning_rates[feat] = self.compute_feature_learning_rate(feat)
        if log.level >= 3:
            log.writeln("learning rate vector: %s" % learning_rates)

        # Solve the quadratic program
        qp.optimize(thedecoder.weights, oweights, learning_rate=learning_rates)

        thedecoder.weights.compact()
        log.write("feature weights: %s\n" % (thedecoder.weights * watch_features))

        # Update weight sum for averaging
        # sum_weights_helper = \sum_{i=0}^n (i \Delta w_i)
        self.sum_weights_helper += self.n_weights * qp.delta_mweights()
        self.n_weights += 1

        # Update feature scales
        if update_feature_scales:
            for instance in qp.instances:
                """u = svector.Vector(instance.hope.mvector)
                for hyp in instance.hyps:
                    u -= hyp.alpha*hyp.mvector
                self.sum_updates2 += u*u"""
                for hyp in instance.hyps:
                    if hyp is not instance.hope: # hyp = instance.hope is a non-update
                        u = instance.hope.mvector - hyp.mvector
                        self.sum_updates2 += hyp.alpha*(u*u)
                        self.n_updates += hyp.alpha

            log.write("feature learning rates: %s\n" % (" ".join("%s=%s" % (f, self.compute_feature_learning_rate(f)) for f in watch_features)))

        theoracle.update(sent.score_comps)

        # make a plain Instance (without forest)
        # we used to designate a hope translation,
        #send_instance = maxmargin.Instance(cur_instance.hyps, hope=cur_instance.hope, instance_id=cur_instance.sentid)
        # but now are letting the other node choose.
        send_instances = []
        for instance in instances:
            if hasattr(instance, "goal") and instance.goal:
                send_instances.append(maxmargin.Instance(instance.hyps, instance_id=instance.sentid))

        assert len(send_instances) == 1
        return send_instances[0]

decoder_errors = 0
def process(sent):
    # Add an flen attribute that gives the length of the input sentence.
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

    # Augment forest with oracle features
    # this is overkill if we aren't going to search for hope/fear
    goal.rescore(theoracle.models, thedecoder.weights, add=True)

    best_vector, best = decoder.get_nbest(goal, 1)[0]
    best_mvector = theoracle.clean(best_vector)
    best_ovector = theoracle.finish(best_vector, best)
    best_loss = theoracle.make_weights(additive="sentence").dot(best_ovector)
    log.writeln("best hyp: %s %s cost=%s loss=%s"  % (" ".join(sym.tostring(e) for e in best), best_vector, thedecoder.weights.dot(best_mvector), best_loss))

    sent.score_comps = best_ovector
    sent.ewords = [sym.tostring(e) for e in best]

    return goal

if __name__ == "__main__":
    gc.set_threshold(100000,10,10)
    
    import optparse
    optparser = optparse.OptionParser()
    # Most of these aren't actually used here...ugly
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
        sys.stderr.write("usage: trainer_slave.py config-file [options...]\n")
        sys.exit(1)

    if log.level >= 1:
        log.write("Reading configuration from %s\n" % configfilename)
    execfile(configfilename)

    opts, args = optparser.parse_args(args=sys.argv[2:])

    maxmargin.watch_features = watch_features

    theoracle = oracle.Oracle(order=4, variant=opts.bleuvariant, oracledoc_size=10)
    thedecoder = make_decoder()
    thelearner = Learner()
    weight_stack = []

    if log.level >= 1:
        gc.collect()
        log.write("all structures loaded, memory=%s\n" % (monitor.memory()))

    comm = MPI.Comm.Get_parent()
    log.prefix = '[%s] ' % (comm.Get_rank(),)

    instances = []
    while True:
        msg = comm.recv()

        if msg[0] == 'train':
            sent = msg[1]
            goal = process(sent)
            instances.append(ForestInstance(sent.id, goal))

            while comm.Iprobe(tag=1):
                msg = comm.recv(tag=1)
                if msg[0] == 'update':
                    log.writeln("receive update for sentence %s" % msg[1].id)
                    instances.append(msg[1].instance)
            
            instance = thelearner.train(sent, instances)
            sent.instance = instance # it would be nicer if sent and instance were the same object
            comm.send(sent, dest=0)
            instances = []

        elif msg[0] == 'translate':
            sent = msg[1]
            process(sent)
            comm.send(sent, dest=0)

        elif msg[0] == 'gather-weights':
            # Average weights (Daume trick)
            sum_weights = float(thelearner.n_weights) * thedecoder.weights - thelearner.sum_weights_helper
            sum_weights.compact()
            log.writeln("send summed weights")
            comm.gather(sum_weights, root=0)
            comm.gather(thelearner.n_weights, root=0)

        elif msg[0] == 'push-weights':
            weight_stack.append(svector.Vector(thedecoder.weights))
            thedecoder.weights = msg[1]
            log.writeln("receive weights: %s" % (thedecoder.weights * watch_features))

        elif msg[0] == 'pop-weights':
            thedecoder.weights = weight_stack.pop()
            log.writeln("restore weights: %s" % (thedecoder.weights * watch_features))

        elif msg[0] == 'end':
            break

        else:
            log.writeln("unknown command: %s" % (msg[0],))

