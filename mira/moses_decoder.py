import sys
import xmlrpclib
import itertools, operator
import re
import optparse

import log
import svector
import forest
import rule, sym
import oracle

scores_re = re.compile(r"core=\((.*?)\)\s+(.*)")

def make_forest(fieldss):
    nodes = {}
    goal_ids = set()
    for fields in fieldss:
        node_id = fields['hyp']
        if node_id not in nodes:
            nodes[node_id] = forest.Item(sym.fromtag('PHRASE'), 0, 0, [])
        node = nodes[node_id]

        if node_id == 0:
            r = rule.Rule(sym.fromtag('PHRASE'), rule.Phrase([]), rule.Phrase([]))
            node.deds.append(forest.Deduction((), r, svector.Vector()))
        else:
            m = scores_re.match(fields['scores'])
            core_values = [float(x) for x in m.group(1).split(',')]
            dcost = svector.Vector(m.group(2).encode('utf8'))
            for i, x in enumerate(core_values):
                dcost["_core%d" % i] = x

            back = int(fields['back'])
            ant = nodes[back]
            f = fields['src-phrase'].encode('utf8').split()
            e = fields['tgt-phrase'].encode('utf8').split()
            if len(f) != int(fields['cover-end']) - int(fields['cover-start']) + 1:
                sys.stderr.write("warning: French phrase length didn't match covered length\n")

            f = rule.Phrase([sym.setindex(sym.fromtag('PHRASE'), 1)] + f)
            e = rule.Phrase([sym.setindex(sym.fromtag('PHRASE'), 1)] + e)
            r = rule.Rule(sym.fromtag('PHRASE'), f, e)

            ded = forest.Deduction((ant,), r, dcost)
            node.deds.append(ded)

            if int(fields['forward']) < 0: # goal
                goal_ids.add(node_id)

    goal = forest.Item(None, 0, 0, [])
    for node_id in goal_ids:
        goal.deds.append(forest.Deduction((nodes[node_id],), None, svector.Vector()))
    return goal

class Decoder(object):
    def __init__(self, url, n_core_features):
        self.weights = None
        self.prev_weights = None
        self.server = xmlrpclib.ServerProxy(url)
        self.n_core_features = n_core_features
        log.write("connected to % s\n" % url)

    def send_weights(self):
        #log.write("prev weights: %s\n" % self.prev_weights)
        #log.write("weights: %s\n" % self.weights)
        if self.prev_weights is None:
            weights = self.weights
        else:
            weights = self.weights - self.prev_weights
            weights.compact()

        core_weights = [0.] * self.n_core_features
        sparse_weights = svector.Vector()
        for feature in weights:
            if not feature.startswith('_core'):
                sparse_weights[feature] = -weights[feature]
            else:
                i = int(feature[5:])
                core_weights[i] = -weights[feature]
        request = {'core-weights' : ','.join(str(x) for x in core_weights),
                   'sparse-weights': str(sparse_weights)}

        if self.prev_weights is None:
            log.write("setWeights(%s)\n" % request)
            self.server.setWeights(request)
        else:
            log.write("addWeights(%s)\n" % request)
            self.server.addWeights(request)
        self.prev_weights = svector.Vector(self.weights)

    def prepare_input(self, input):
        pass

    def process_output(self, sent, outforest):
        pass

    def translate(self, input):
        """input: any object that has an attribute 'words' which is a list of numberized French words. and an 'id' attribute. and an 'instruction' attribute
           output: a forest"""

        self.send_weights()

        request = {'text' : " ".join(input.words),
                   'id' : input.id,
                   'sg' : True}
        log.write("translate(%s)\n" % (request,))
        response = self.server.translate(request)
        log.write("received response\n")

        f = make_forest(response['sg'])

        for item in f.bottomup():
            for ded in item.deds:
                for feature in opts.delete_features.split(','):
                    del ded.dcost[feature]

        log.write("converted to forest\n")
        f.reweight(self.weights) # because make_forest doesn't compute viterbi

        return f

# already created by trainer.py
# optparser = optparse.OptionParser()

optparser.add_option("-d", "--decoder", dest="decoder")
optparser.add_option("-w", "--feature-weights", dest="feature_weights")
optparser.add_option("--delete-features", dest="delete_features")
optparser.add_option("--initial-learning-rate", dest="initial_learning_rate", default=1.)
optparser.add_option("--learning-rate-decay", dest="learning_rate_decay", default=0.01)
optparser.add_option("--initial-feature-learning-rates", dest="initial_feature_learning_rates")
optparser.add_option("--n-core-features", dest="n_core_features", type=int)

opts, _ = optparser.parse_args()

if not opts.decoder:
    raise ValueError("-d or --decoder must be specified")
if not opts.n_core_features:
    raise ValueError("--n-core-features must be specified")

initial_learning_rate = opts.initial_learning_rate
learning_rate_decay = opts.learning_rate_decay
if opts.initial_feature_learning_rates:
    if '=' in opts.initial_feature_learning_rates:
        initial_feature_learning_rates = svector.Vector(opts.initial_feature_learning_rates)
    else:
        initial_feature_learning_rates = svector.Vector(open(opts.initial_feature_learning_rates).read())
        

watch_features = svector.Vector()
for i in xrange(opts.n_core_features):
    watch_features['_core%d' % i] = 1

def make_decoder():
    thedecoder = Decoder(opts.decoder, opts.n_core_features)
    if opts.feature_weights:
        if '=' in opts.feature_weights:
            thedecoder.weights = -svector.Vector(opts.feature_weights)
        else:
            thedecoder.weights = -svector.Vector(open(opts.feature_weights).read())
    else:
        thedecoder.weights = svector.Vector()
    return thedecoder
