### A wrapper around the SBMT decoder to make it masquerade as a Hiero decoder.

import sym, os, os.path, signal
import subprocess
import threading, Queue
import optparse
import itertools
import rule
import forest
import svector
import log

class Popen(subprocess.Popen):
    def __init__(self, *args, **kwargs):
        self.send_queue = Queue.Queue()

        send_thread = threading.Thread(target=self._sender)
        send_thread.daemon = True
        send_thread.start()

        subprocess.Popen.__init__(self, *args, **kwargs)

    def _sender(self):
        while True:
            line = self.send_queue.get()
            self.stdin.write(line)
            self.send_queue.task_done()

    def send(self, s):
        self.send_queue.put(s)

    def recvline(self):
        return self.stdout.readline()

if True: # pychecker support
    import sys
    if sys.argv[0].endswith('checker.py'):
        optparser = optparse.OptionParser() # for pychecker standalone testing.

def cstr_escape_nows(s):
    return s.replace('\\','\\\\').replace('"','\\"')

optparser.add_option("-d", "--decoder", dest="decoder")
optparser.add_option("-w", "--feature-weights", dest="feature_weights")
optparser.add_option("--delete-features", dest="delete_features")
optparser.add_option("-g", "--gars-dir", dest="gars_dir")

optparser.add_option("--feature-scaling", dest="update_feature_scales", default=None)
optparser.add_option("--initial-learning-rate", dest="initial_learning_rate", default=1., type=float)
optparser.add_option("--initial-feature-learning-rates", dest="initial_feature_learning_rates")
optparser.add_option("--learning-rate-decay", dest="learning_rate_decay", default=0.01, type=float)
optparser.add_option("--unit-learning-rate", dest="unit_learning_rate", default=0.01, type=float)
optparser.add_option("--max-learning-rate", dest="max_learning_rate", default=None, type=float)

# these options don't do anything
optparser.add_option("--feature-scales", dest="feature_scales")
optparser.add_option("--positive-features", dest="positive_features")
optparser.add_option("--weights-nodiff", dest="weights_nodiff")
badforestdir='logs/'
opts, _ = optparser.parse_args()
default_delta=not opts.weights_nodiff
#default_delta=False #TODO: test delta

def sbmt_vector(s):
    v = svector.Vector()
    if s:
        for featvalue in s.split(","):
            feat,value = featvalue.split(":",1)
            v[feat]=float(value)
    return v

# Initial feature weights.
feature_weights = svector.Vector()
if opts.feature_weights:
    try:
        feature_weights = sbmt_vector(opts.feature_weights)
    except:
        try:
            feature_weights = sbmt_vector(file(opts.feature_weights).read())
        except:
            raise Exception("couldn't obtain feature weights from %s\n" % opts.feature_weights)

if opts.update_feature_scales in [None, "gauss-newton", "arow"]:
    update_feature_scales = opts.update_feature_scales
else:
    log.writeln("warning: unknown value for --scale-features: %s" % opts.update_feature_scales)

# Scaling factors for feature values as seen by the trainer. Think of
# this as a learning rate that can be independently adjusted for each
# feature. Ideally we want all the scaled feature values to have
# similar magnitudes.
if opts.feature_scales:
    log.writeln("warning: --feature-scales is no longer supported")

# Features that are constrained to be >= 0. We're not using a very
# smart method to enforce this constraint, so too many could paralyze
# the trainer. It's probably a good idea to initialize these features
# to nonzero values.
positive_features = []
if opts.positive_features:
    log.writeln("warning: --positive-features is no longer supported")

# Features that SBMT emits but won't respond to weights for.
# They need to be suppressed.
delete_features = []
if opts.delete_features:
    delete_features = opts.delete_features.split(',')

unit_learning_rate = opts.unit_learning_rate
max_learning_rate = opts.max_learning_rate
initial_learning_rate = opts.initial_learning_rate
learning_rate_decay = opts.learning_rate_decay

if opts.initial_feature_learning_rates:
    initial_feature_learning_rates = sbmt_vector(opts.initial_feature_learning_rates)

### We must instantiate two objects, thereader and thedecoder.

class Sentence(object):
    def __init__(self, id=-1, instruction='', refs=[], fwords=[]):
        self.id=id
        self.instruction=instruction.rstrip()
        self.refs=list(refs)
        self.fwords=list(fwords)
        self.n = len(self.fwords)

    def __str__(self):
        return "sent=%s" % (self.id)

    def __repr__(self):
        return "%s fwords={{{%s}}} refs={{{%s}}} ins={{{%s}}}"%(self," ".join(self.fwords),'\n'.join(' '.join(x) for x in self.refs),self.instruction)

    def compute_distance(self):
        # this could be replaced with a real distance function for the lattice
        distance = {}
        for i in xrange(self.n):
            for j in xrange(i, self.n+1):
                distance[i,j] = j-i
        return distance

def thereader(infile, reffiles):
    """Input: the input file given by the user on the command-line, and the reference files.
       Output: iterator over Sentence objects.
       The Sentence objects must have an id attribute (their 0-based sentence number) and a words attribute (a list of words-as-strings).
       """
    def prefix_map(inp):
        mp = {}
        for line in open(inp):
            if line[0] != '#':
                v = line.rstrip('\n').split('\t')
                mp[int(v[0])] = v[2]
        return mp
    pmap = prefix_map(os.path.join(opts.gars_dir,'corpus.map'))
    for (li, (inline, reflines)) in enumerate(itertools.izip(infile, itertools.izip(*reffiles))):
        yield Sentence(li,
                       file(os.path.join(opts.gars_dir,pmap[li+1],'decode.ins')).read(),
                       [refline.split() for refline in reflines], 
                       inline.split())
        # We assume that the instructions to decode sentence 1234
        # are stored in tmp/12/34/decode.ins
#        s.instruction = file(os.path.join(opts.gars_dir, "tmp", "%02d"%((li+1)/100), "%02d"%((li+1)%100), "decode.ins")).read()

class Decoder(object):
    def __init__(self):
        self.child = None
        self.weights = None
        self.start_decoder()

    def send_weights(self,delta=None,input=''):
        if delta is None:
            delta=default_delta
        if len(self.oldweights)==0:
            delta=False # looking to avoid any weird bug from decoder's default weight vector (note: excluding 0 items is risky too)
        w=self.weights
        fmt="%s:%+g" if delta else "%s:%g"
        cmd="weights"
        keep=lambda x: True
        if delta:
            keep=lambda x: abs(x)!=0.
            cmd+=" diff"
            w = w - self.oldweights
        weightstr=",".join(fmt % (cstr_escape_nows(k),v) for (k,v) in w.iteritems() if keep(v))
            #FIXME: should non-delta weights omit 0? should be ok, except crazy lm (unk?) weight from feature semantics
        self.send_instruction('%s "%s";' % (cmd,weightstr),input)
        self.oldweights = svector.Vector(self.weights)

    def start_decoder(self):
        if self.child:
            log.writeln("stopping decoder")
            try:
                if not self.child.returncode:
                    self.child.stdin.close()
                    self.child.stdout.close()
                    # "Warning: This will deadlock when using stdout=PIPE and/or stderr=PIPE and the child process generates enough output to a pipe such that it blocks waiting for the OS pipe buffer to accept more data."
                    # but closing stdout means that it should get SIGPIPE if process has buffered output or writes more. so OK.
                    self.child.wait()
                else:
                    os.kill(self.child.pid, signal.SIGKILL)
                    self.child=None
            except:
                pass
        self.child = Popen(
            [opts.decoder],
            cwd=opts.gars_dir,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE)
        log.writeln("started decoder subprocess=%s at %s" % (self.child.pid,log.datetoday()))
        self.decoder_age = 0
        self.oldweights = svector.Vector()

    def create_forest(self,line):
        try:
            self.forest = forest.forest_from_text(line, delete_words=['@UNKNOWN@'])
            return True
        except forest.TreeFormatException:
            self.forest = None
            return False

    def instruct(self, input):
        pg = "pop-grammar;"
        if input.instruction.endswith(pg):
            spre, spost = input.instruction[:-len(pg)].rstrip(), pg
        else:
            spre, spost = s, ''

        self.send_instruction(spre, input)
        log.writeln("reading forest from decoder\n")
        r = self.child.recvline()
        self.send_instruction(spost, input)
        return r

    def send_instruction(self, s, input=''):
        s = s.rstrip()
        if len(s):
            log.writeln("sending instruction: %s for %s" % (s.rstrip(), input))
            self.child.send(s+"\n")

    def prepare_input(self, input):
        pass

    def process_output(self, sent, outforest):
        pass

    def translate(self, input):
        """input: any object that has an attribute 'words' which is a list of numberized French words. and an 'id' attribute. and an 'instruction' attribute
           output: a forest"""

        if self.decoder_age >= 100:
            self.start_decoder()

        restarts = 0
        self.decoder_age += 1
        outforest=""
        while restarts <= 3:
            try:
                self.send_weights(input=input)
                outforest = self.instruct(input)
                if outforest == "" or not self.create_forest(outforest) or self.child.poll() is not None:
                    continue
                else:
                    break
                # graehl->pust: careful - restarts += 1 doesn't happen on continue. infinite loop possible if decoder really outputs no forest (I think you changed it so a dummy forest is output, so this may be what you want? just bad for error reporting if you hang forever)
            except:
                lastexcept=log.strexcept(True)
                log.writeln("CAUGHT exception: %s" % lastexcept)
                pass
            restarts += 1
            if restarts <= 3:
                log.writeln("restarting decoder")
                self.start_decoder()
            else:
                self.start_decoder()
                #raise Exception("too many decoder restarts for %s, giving up - last was: %s"%(input,lastexcept))
                #don't raise because of global 100-retries limit in trainer.py
                log.write("too many decoder restarts, giving up on exception %s:\n%s\nwith weights:\n%s\n" % (lastexcept,repr(input),self.weights))
                self.create_forest("(0<noparse:1> )")


        # self.send_instruction('weights diff "%s";' % weightstring, input)
        # self.oldweights = svector.Vector(self.weights)

        # self.send_instruction(input.instruction,input)
        # outforest = self.child.recvline()

        # restarts = 0
        # while outforest == "" or self.child.poll() is not None:
        #     log.writeln("restarting decoder")
        #     self.start_decoder()
        #     if restarts > 3:
        #         raise Exception("too many decoder restarts, giving up")
        #     self.send_instruction('weights "%s";' % weightstring, input)
        #     self.send_instruction(input.instruction, input)
        #     outforest = self.child.recvline()
        #     restarts += 1

        log.writeln("received forest: %s...%s for %s" % (outforest[:80],outforest[-80:], input))
        #sys.stderr.write("received forest: %s\n" % (outforest,))

        # try:
        #     f = forest.forest_from_text(outforest, delete_words=['@UNKNOWN@'])
        # except forest.TreeFormatException:
        #     badforestf='%s/badforest.%s'%(badforestdir,input.id)
        #     log.write("dumping bad forest to %s\n" % (badforestf,))
        #     forestfile = file(badforestf, "w")
        #     forestfile.write(outforest)
        #     forestfile.close()
        #     raise

        f = self.forest
        self.forest = None
        #sys.stderr.write("internal forest: %s\n" % (forest.forest_to_text(f, mode='english')))

        for item in f.bottomup():
            for ded in item.deds:
                # replace rule's French side with correct number of French words
                # we don't even bother to use the right number of variables
                ded.rule = rule.Rule(ded.rule.lhs,
                                     rule.Phrase([sym.fromstring('<foreign-word>')]*int(ded.dcost['foreign-length'])),
                                     ded.rule.e)

                for feature in delete_features:
                    del ded.dcost[feature]

        f.reweight(self.weights) # because forest_from_text doesn't compute viterbi

        return f

def make_decoder():
    thedecoder = Decoder()
    thedecoder.weights = feature_weights
    return thedecoder


