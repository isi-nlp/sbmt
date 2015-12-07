import svector
import log
verbosity = 1
watch_features = svector.Vector()

class QuadraticProgram(object):
    def __init__(self, instances=()):
        self.instances = []
        for instance in instances:
            self.add_instance(instance)

    def add_instance(self, instance):
        """Add an instance (a training example) to this QP."""
        instance.qp = self
        if instance.instance_id is None:
            instance.instance_id = len(self.instances)
        self.instances.append(instance)

    def optimize(self, mweights, oweights, learning_rate=1.):
        """Optimize the model weights (mweights), using a set of
           oracle weights (oweights) to compute the loss function. The
           dot product of mweights and the model features of a
           hypothesis is its model cost (lower is better), and the dot
           product of oweights and the oracle features of a hypothesis
           is its loss (lower is better).

           The learning_rate can be an svector.Vector, but it cannot
           have any zero rates.
           """

        self.mweights = mweights
        self.oweights = oweights
        self.learning_rate = learning_rate

        if verbosity >= 2:
            log.writeln("begin optimization")

        # Initialize QP by selecting a hope hypothesis for each instance
        for instance in self.instances:
            if verbosity >= 3:
                log.writeln("instance %s:" % (instance.instance_id,))
            instance._new_hope(instance.get_hope())

        if verbosity >= 2:
            for instance in self.instances:
                log.writeln("instance %s:" % (instance.instance_id,))
                for hyp in instance.hyps:
                    log.writeln("  hyp %s: %s" % (hyp.hyp_id, hyp))

        if verbosity >= 1:
            if len(watch_features):
                log.writeln("initial weights: %s" % (self.mweights * watch_features))
            log.writeln("objective function: %s" % self.objective())

        again = True
        while again:
            again = False
            for instance in self.instances:
                if instance._new_fear():
                    again = True

            self._optimize_workingset()

        if verbosity >= 1:
            if len(watch_features) > 0:
                log.writeln("final weights: %s" % (self.mweights * watch_features))
            log.writeln("objective function: %s" % self.objective())

        return mweights

    def delta_mweights(self):
        dmweights = svector.Vector()
        for instance in self.instances:
            dmweights += self.learning_rate * instance.hope.mvector
            for hyp in instance.hyps:
                dmweights -= hyp.alpha * self.learning_rate * hyp.mvector
        return -dmweights

    def objective(self):
        dmweights = self.delta_mweights()
        obj = 0.5 * dmweights.dot(dmweights/self.learning_rate)
        for instance in self.instances:
            if verbosity >= 3:
                log.writeln("instance %s:" % instance.instance_id)
            obj += max(instance.violation(hyp) for hyp in instance.hyps)
            if verbosity >= 3:
                for hyp in instance.hyps:
                    log.writeln("  hyp %s: alpha=%s violation=%s" % (hyp.hyp_id, hyp.alpha, instance.violation(hyp)))
        return obj

    def _optimize_workingset(self, max_iterations=1000):
        """Optimize mweights for a fixed working set of hyps,
           using oweights to compute loss function"""
        iterations = 0
        again = True
        while again:
            if verbosity >= 5:
                log.writeln("SMO iteration %d" % iterations)
            again = False
            # shuffle instances?
            for instance in self.instances:
                if len(instance.hyps) < 2:
                    continue

                if verbosity >= 5:
                    log.writeln("try to improve instance %s:" % (instance.instance_id))

                # cache violation inside hyps, needed by both _select_pair and _optimize_pair
                for hyp in instance.hyps:
                    hyp.violation = instance.violation(hyp)

                hyps = instance._select_pair()
                if hyps is None:
                    if verbosity >= 5:
                        log.writeln("all KKT conditions (almost) satisfied, do nothing")
                    continue

                if instance._optimize_pair(*hyps):
                    again = True

            iterations += 1
            if iterations >= max_iterations:
                if verbosity >= 1:
                    log.writeln("SMO: giving up")
                break
        if verbosity >= 1:
            log.writeln("SMO: finished in %d iterations" % iterations)
        if verbosity >= 4:
            if len(watch_features) > 0:
                log.writeln("new weights: %s" % (self.mweights * watch_features))
            log.writeln("objective function: %s" % self.objective())

class Instance(object):
    """A training example (e.g., an input sentence). It has a 'hope'
       hypothesis, which is normally the correct output, and a working
       set of competing hypotheses."""
    def __init__(self, hyps=(), instance_id=None, hope=None):
        self.hyps = []
        self.hope = hope
        self.instance_id = instance_id
        for hyp in hyps:
            self.add_hyp(hyp)

    def add_hyp(self, hyp, alpha=0.):
        """Add a new hypothesis to this instance."""
        if hyp not in self.hyps:
            hyp.alpha = alpha
            hyp.hyp_id = len(self.hyps)
            self.hyps.append(hyp)

    def _new_fear(self, epsilon=0.01):
        """Possibly add a new fear hypothesis to the QP."""
        try:
            fear = self.get_fear()
        except NotImplementedError:
            return False

        violation = self.violation(fear)
        for hyp in self.hyps:
            if violation <= self.violation(hyp) + epsilon:
                return False
        self.add_hyp(fear)
        if verbosity >= 2:
            log.writeln("instance %s new hyp %s: %s violation=%s" % (self.instance_id, fear.hyp_id, fear, violation))
        return True

    def get_fear(self):
        """The separation oracle. Override this method to allow new
        fear hypotheses to be added to the QP. For a given mweights,
        the fear hypothesis is that which has maximum
        violation (hinge loss)."""
        raise NotImplementedError()

    def _new_hope(self, new_hope):
        """Switch to a new hope hypothesis."""
        old_hope = self.hope
        self.add_hyp(new_hope)
        self.hope = new_hope

        if verbosity >= 2:
            log.writeln("hope candidates:")
            for hyp in self.hyps:
                if hyp is new_hope:
                    code = "+"
                elif hyp is old_hope:
                    code = "-"
                else:
                    code = " "
                log.writeln("%s hyp %s: cost=%s loss=%s alpha=%s" % (code, hyp.hyp_id, self.qp.mweights.dot(hyp.mvector), self.qp.oweights.dot(hyp.ovector), hyp.alpha))

        new_hope.alpha += 1.
        if old_hope:
            # Preserve identity:
            # mweights = mweights_0 - \sum_i hope_i + \sum_j alpha_ij hyp_ij
            old_hope.alpha -= 1.

        # Adjust alphas to be consistent with current weights
        if old_hope and new_hope is not old_hope:
            if verbosity >= 3:
                log.writeln("changing hope:")
                log.writeln("  old hope: %s alpha=%s" % (old_hope, old_hope.alpha))
                log.writeln("  new hope: %s alpha=%s" % (new_hope, new_hope.alpha))

            # Just in case one of the alphas went outside [0,1], immediately
            # optimize them
            old_hope.violation = self.violation(old_hope)
            new_hope.violation = self.violation(new_hope)
            self._optimize_pair(old_hope, new_hope)
            if verbosity >= 1:
                log.writeln("adjust alphas: old hope %s, new hope %s" % (old_hope.alpha, new_hope.alpha))
                log.writeln("adjust weights: %s" % (self.qp.mweights * watch_features))

    def reset(self):
        self.hope = None
        for hyp in self.hyps:
            hyp.alpha = 0.

    def get_hope(self):
        """Select a hope hypothesis. This can either be an existing
        hypothesis or a new one. The default implementation, which
        can be overridden, is to select the hypothesis with lowest loss."""
        _, hope = min((self.qp.oweights.dot(hyp.ovector), hyp) for hyp in self.hyps)
        return hope

    def violation(self, hyp):
        """The hinge loss, which we are trying to minimize. Equal to
        the loss (how much worse is hyp than hope) minus the margin
        (how much does the model prefer hope over hyp)."""
        # assume that mweights.dot is distributive but that oweights might not be
        loss = -self.qp.oweights.dot(self.hope.ovector)+self.qp.oweights.dot(hyp.ovector)
        margin = -self.qp.mweights.dot(self.hope.mvector-hyp.mvector)
        return loss-margin

    def _optimize_pair(self, hyp2, hyp1):
        """Try to improve the objective function by finding some pair
           of hypotheses and moving weight (alpha) from one to the
           other, keeping the total weight constant."""

        # this is the direction of the update to self.mweights
        update = hyp2.mvector-hyp1.mvector
        if verbosity >= 5:
            log.writeln("update direction: %s" % update)

        # now we figure out what step size is needed to minimize the combined violation
        denom = update.dot(self.qp.learning_rate*update)
        if denom > 0.:
            delta = (hyp2.violation-hyp1.violation)/denom
            # but don't let any alpha go negative
            assert -hyp2.alpha <= hyp1.alpha
            clipped_delta = max(-hyp2.alpha, min(hyp1.alpha, delta))
            if verbosity >= 5:
                log.writeln("delta: %s->%s" % (delta, clipped_delta))
            delta = clipped_delta
        else:
            log.writeln("warning: update direction would be useless, not updating")
            return False

        hyp2.alpha += delta
        hyp1.alpha -= delta
        self.qp.mweights += delta*self.qp.learning_rate*update
        if verbosity >= 5:
            if len(watch_features) > 0:
                log.writeln("new weights: %s" % (self.qp.mweights * watch_features))
            log.writeln("objective function: %s" % self.qp.objective())
        return True

    def _select_pair(self, epsilon=0.01):
        """Find a pair of hypotheses that violates one of the KKT conditions:
             alpha_i = 0 => violation_i is not the max
             alpha_i > 0 => violation_i is the max
           From Ben Taskar's PhD thesis, p. 80."""
        # shuffle hyps?
        for hyp in self.hyps:
            if verbosity >= 5:
                log.writeln("hyp %s" % hyp.hyp_id)
            violation_max = max(hyp1.violation for hyp1 in self.hyps if hyp1 is not hyp)
            if verbosity >= 5:
                log.writeln("  max violation of other hyps: %s" % violation_max)

            if hyp.alpha == 0 and hyp.violation > violation_max + epsilon:
                # hyp is the worst violator but has no weight yet,
                # so find someone to take weight from
                for hyp1 in self.hyps:
                    if hyp1 is not hyp and hyp1.alpha > 0:
                        if verbosity >= 5:
                            log.writeln("hyp %s alpha = %s -> fear hyp %s alpha %s" % (hyp1.hyp_id, hyp1.alpha, hyp.hyp_id, hyp.alpha))
                        return hyp, hyp1

            if hyp.alpha > 0 and hyp.violation < violation_max - epsilon:
                # hyp has weight but is not the worst violator,
                # so find a worse violator to give weight to
                for hyp1 in self.hyps:
                    if hyp1 is not hyp and hyp1.violation > hyp.violation + epsilon:
                        if verbosity >= 5:
                            log.writeln("hyp %s alpha = %s -> worse violator hyp %s alpha %s" % (hyp.hyp_id, hyp.alpha, hyp1.hyp_id, hyp1.alpha))
                        return hyp1, hyp

class Hypothesis(object):
    """A hypothesis is a possible output for an instance. It consists
       of a model vector (mvector) and an oracle vector (ovector).
       The mvector, when dotted with the model weights, yields a model
       cost (lower is better).  The ovector, when dotted with the
       oracle weights, yields a loss (lower is better)."""

    def __init__(self, mvector, ovector, hyp_id=None):
        self.mvector = mvector
        self.ovector = ovector
        self.hyp_id = hyp_id

    def __str__(self):
        return "%s %s" % (self.mvector, self.ovector)

class MIRALearner(object):
    def __init__(self):
        self.mweights = svector.Vector()
        self.oweights = svector.Vector("loss=1")
        self.learning_rate = 0.1

    def receive(self, instances):
        qp = QuadraticProgram(instances)
        qp.optimize(self.mweights, self.oweights, self.learning_rate)

if __name__ == "__main__":
    """instances = [
        Instance([Hypothesis(svector.Vector("a=1 b=2"), svector.Vector("loss=1")),
                  Hypothesis(svector.Vector("a=1 b=1"), svector.Vector("loss=2")),
                  Hypothesis(svector.Vector("a=0 b=1"), svector.Vector("loss=3"))]),
        Instance([Hypothesis(svector.Vector("a=2 b=1"), svector.Vector("loss=2")),
                  Hypothesis(svector.Vector("a=1 b=2"), svector.Vector("loss=4")),
                  Hypothesis(svector.Vector("a=2 b=0"), svector.Vector("loss=5")),
                  Hypothesis(svector.Vector("a=1 b=1"), svector.Vector("loss=3"))])]"""

    #qp = QuadraticProgram(instances)
    qp = pickle.load(open("maxmargin.dump"))
    mweights = svector.Vector()
    oweights = svector.Vector("loss=1")
    learning_rate = 0.01
    qp.optimize(mweights, oweights, learning_rate)
