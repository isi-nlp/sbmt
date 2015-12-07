#!/usr/bin/env pypy
#!/usr/bin/env pypy
# mbr_decode.py
# Steve DeNeefe, Jun 19, 2006
#
# based on bleu+1.py script by David Chiang on Apr 4, 2006
# Jon Graehl: option for linear expected-bleu (DeNero's idea) instead of quadratic, cost base, per system normalization
import os
dbg=os.environ.get('DEBUG')
def dump(l):
    if dbg: pprint.pprint(l,sys.stderr)

def pause():
    if dbg: pdb.set_trace()

import pprint
import pdb
import sys, itertools, math
import optparse
#sys.path.append("/home/rcf-12/graehl/projects/syscom/bin/score")
sys.path.append("/auto/nlg-01/chiangd/tools/")
import bleu
bleu.preserve_case = True
bleu.nist_tokenize = False
bleu.clip_len = False

def parse_name_weight(p,colon=':'):
    l=p.partition(colon)
    return (l,float(l[2]))

# return hash name:value from n1:v1,n2:v2 ... (comma sep)
def parse_weights(str,colon=':',comma=','):
    return dict(parse_name_weight(p,colon=colon) for p in str.split(comma))

def parse_weights_announce(name,str,colon=':',comma=','):
    sys.stdout.write(", %s='%s'"%(name,str))
    ret=parse_weights(str,colon=colon,comma=comma)
    for n,w in ret.iteritems():
        sys.stdout.write(" %s=%f" % (n,w))
    sys.stdout.write("\n")
    return ret
    

def bleu_single(test,cookedrefs,n=4,addprec=1):
    comps=bleu.cook_test(test,cookedrefs,n=n)
    p=1.
    for k in xrange(n):
        p *= float(comps['correct'][k]+addprec)/(comps['guess'][k]+addprec)
    p = p ** (1./n)
    if 0 < comps['testlen'] < comps['reflen']:
        p *= math.exp(1-float(comps['reflen'])/comps['testlen'])
    return p

def score_vs_cooked(test,cookedrefs,n=4,addprec=1):
    return bleu_single(test,cookedrefs,n,addprec)
# note: using the below would allow "closest", but that's meaningless for mbr (always equal to your own length)
#    return bleu.score_cooked([bleu.cook_test(test,cookedrefs,n=n)],n=n,addprec=addprec)


    
def collapse_reflen(reflen,eff_ref_len="average"):
    if eff_ref_len == "shortest":
        reflen = min(reflen)
    elif eff_ref_len == "average":
        reflen = float(sum(reflen))/len(reflen)
    return reflen

def normalize(probs):
    sum=0
    for p in probs:
        sum+=p
    if sum == 1:
        return probs
    return [p/sum for p in probs]

def isclose(a,b,eps=1e-4):
    return b-eps <= a <= b+eps

def cook_expected_ref(refs,n=4,p_refs=None,eff_ref_len="average"):
    '''same input as bleu.cook_refs, but maxcounts is avg. rather than max.
    p_refs is a parallel (to refs) list of weights.  uniform 1/n will be used if
    None'''
    
    if p_refs == None:
        l=len(refs)
        p_refs=[1.0/l]*l
    assert isclose(sum(p_refs),1)
        
    reflen = []
    #    ecount=collections.defaultdict(float)
    ecount = {}

    for ref,p in itertools.izip(refs,p_refs):
        rl, counts = bleu.precook(ref, n)
        reflen.append(rl)
        for (ngram,count) in counts.iteritems():
            #ecounts[ngram] += p*count
            ecount[ngram] = ecount.get(ngram,0)+p*count

    return collapse_reflen(reflen,eff_ref_len),ecount
    
    
def mbr_best(lines, nbest, cost_weighting, rank_limit, rank_weight, sys_weights, sys_cost_bases=None,fast=False,addprec=1,eff_ref_len="average",n=4,cost_base=None,normalize_cost_base=False,per_system_norm=True):
    if sys_weights == None or len(sys_weights)==0:
        no_syswt=True
        sys_weights={}
    else:
        no_syswt=False
        sumsw=sum(sys_weights.itervalues())
        sys_weights=dict((s,(w/sumsw)) for s,w in sys_weights.iteritems())

        
    use_cost_base = (cost_base != None or sys_cost_bases != None)
    if use_cost_base and sys_cost_bases == None:
        sys_cost_bases = {}
        
    if nbest:
        start = 6  # added system also to beginning
    else:
        start = 1

    entries_per_system = {}
    best_system_score = {} # these are set by first/last in input, rather than assuming more positive -> better
    worst_system_score = {}
    max_system_cost = {}
#    cookedrefs = []
    ref_probs = []
    ref_syss = []
    splits=[line.split(None,start) for line in lines]
    hyps=[bleu.precook(s[start]) for s in splits]
    sump_sys = {}
    for split_ref in splits:                
        sysname = split_ref[0]
        entries_per_system[sysname] = entries_per_system.get(sysname, 0) + 1
        if nbest:
            score = float(split_ref[5])
            cost = -score
            if sysname not in best_system_score:
                best_system_score[sysname]=score
            worst_system_score[sysname]=score
            if max_system_cost.get(sysname, 0) < cost:
                max_system_cost[sysname] = cost
                
#    diff_system_score = dict((s,w-best_system_score[s]) for s,w in worst_system_score.iteritems())

    sump=0
    for split_ref in splits:
        p = 1.0
        
        sysname = split_ref[0]
        
        if not per_system_norm and sysname in sys_weights:
#            pdb.set_trace()        
            p *= sys_weights[sysname]
            
        if nbest:
            if rank_weight != None:
                rank = int(split_ref[4])
                p *= 1./(rank_weight + rank)
            if use_cost_base and sysname in best_system_score:
                bss=best_system_score[sysname]
                score = float(split_ref[5])-bss
                if normalize_cost_base:
                    diff=bss-worst_system_score[sysname]
                    #diff=diff_system_score[sysname]
                    if diff!=0:
                        score /= diff
                p *= math.pow(sys_cost_bases.get(sysname,cost_base),score)
            if cost_weighting and max_system_cost.get(sysname, 0) > 0.0:
                cost = -float(split_ref[5])
                p *= cost/max_system_cost[sysname]
        sump+=p
        sump_sys[sysname]=sump_sys.get(sysname,0)+p
        ref_probs.append(p)
        ref_syss.append(sysname)
        
    if per_system_norm:
        if no_syswt:
            wsys=1./len(sump_sys)
            sys_weights=dict.fromkeys(sump_sys.iterkeys(),wsys)
#            dump(sys_weights)
        assert isclose(sum(sys_weights.itervalues()),1)
            
        mult_sys=dict((n,(sys_weights[n]/s)) for n,s
                      in sump_sys.iteritems());
#        dump([(sum([p for n,p in zip(ref_syss,ref_probs) if n==sn]),sn,sump_sys[sn],mult_sys[sn]) for sn in sump_sys.keys()])
        ref_probs=[mult_sys[sn]*p for sn,p in itertools.izip(ref_syss,ref_probs)]
        dump(sum(ref_probs))
    elif sump != 1.0:
        oos=1./sump
        ref_probs=[p*oos for p in ref_probs]        

    if fast:
        expected_ref=cook_expected_ref(hyps,n=n,p_refs=ref_probs,eff_ref_len=eff_ref_len)
    else:
        cookedrefs = [ bleu.cook_refs([s[start]],n=n) for s in splits ]
        
        
    max_items = []
    avg_bleu = []
    N=len(hyps)
    for test in xrange(N):
        split_test = splits[test]
        avg_test_bleu = 0.0

        if nbest and rank_limit != None:
            test_rank = int(split_test[4])
            if test_rank > rank_limit:
                avg_bleu.append(avg_test_bleu)
                continue

        if fast:
            avg_test_bleu = score_vs_cooked(hyps[test],expected_ref,n=n,addprec=addprec)
        else:
            for ref in xrange(N):
                split_ref = splits[ref]
                factor = ref_probs[ref]
#                score=1.
                if ref != test: # each system gets to vote for itself
                    score=score_vs_cooked(hyps[test],cookedrefs[ref],n=n,addprec=addprec)
                else:
                    score=1.
                avg_test_bleu += ref_probs[ref]*score
        avg_bleu.append(avg_test_bleu)

        if len(max_items) == 0 or avg_test_bleu == avg_bleu[max_items[0]]:
            max_items.append(test)
        elif avg_test_bleu > avg_bleu[max_items[0]]:
            max_items = []
            max_items.append(test)
    dump(avg_bleu)
#    dump([x/avg_bleu[0] for x in avg_bleu])
    return max_items,avg_bleu


# usage: mbr_decode.py <hyp-file>

if __name__ == "__main__":
    optparser = optparse.OptionParser()
    optparser.add_option("-n", "--nbest-format", action="store_true", dest="nbest", default=False, help="read input files as NBEST files (format: SENT <num> HYP <num> <score> <sentence>)")
    optparser.add_option("-l", "--limit-rank", dest="rank_limit", default=None, help="use this option to limit the chosen hypothesis to at most a specified rank (e.g. pass 1 if you only want 1-best output) -- note that all n-best entries still participate in voting")
    optparser.add_option("-r", "--rank-weight", dest="rank_weight", default=None, help="use this option to set a weight to the rank (higher is lower weight)")
    optparser.add_option("-s", "--system-weight-string", dest="sys_weight_str", default=None, help="use this option to set a per-system weight, e.g. SYS1:.7,SYS2:.3")
    optparser.add_option("-c", "--cost-weighting", action="store_true", dest="cost_weighting", default=False, help="use this option to set the weight of each nbest entry equal to the system cost (for each system the highest cost in the nbest list is normalized to -1)")
    optparser.add_option("-f","--fast-expected-sentence", action="store_true",
                         dest="fast", default=False, help=
                         "use expected-sentence approximate mbr-bleu (linear time instead of quadratic)")
    optparser.add_option("-R", "--ref-len", dest="eff_ref_len",
                         default="average", choices=["average","shortest"], #,"closest"
                         help="shortest or average (reference length for bleu)")
    optparser.add_option("-m", "--max-sents", dest="max_sents", type="int",
                         help="stop after m sents")
    optparser.add_option("-a","--add-prec", dest="add_prec", type="float", default=1, help="in ngram precision #correct/#guessed, add [a] to both numerator and denominator.  recommend a>0 or else 0 precision for any ngram length -> 0 bleu")
    optparser.add_option("-b","--cost-base", dest="cost_base", type="float", default=None, help="alternative to cost weighting: raise (positive) base^cost (then normalize). 1best cost normalized (by subtraction, not division) to 0.  recommend --per-system-norm and --system-cost-base-string for >1 system")
    optparser.add_option("-t", "--system-cost-base-string",dest="cost_bases_str", default=None, help="SYS1:base1,SYS2:base2 like --system-weight-string")
    optparser.add_option("-z", "--normalize-cost-base", action="store_true", default=False, dest="normalize_cost_base",help="difference between first and last cost is normalized to 1 (i.e. best = 0, worst = -1)")
    optparser.add_option("-p", "--per-system-norm", action="store_true", default=False, dest="per_system_norm",help="choose system by uniform or system weights, then choose nbest from system")
    
    (opts, args) = optparser.parse_args()
    rw_num = None
    rl_num = None
    sys.stdout.write("Options: ")
    if opts.nbest:
        sys.stdout.write(" nbest mbr")
        if opts.rank_weight != None:
            sys.stdout.write(", rank weight=%s" % opts.rank_weight)
            rw_num = float(opts.rank_weight)
        else:
            sys.stdout.write(", no rank weighting")

        if opts.cost_weighting:
            sys.stdout.write(", cost weighting")
        else:
            sys.stdout.write(", no cost weighting")

        if opts.rank_limit != None:
            sys.stdout.write(", output rank limit=%s" % opts.rank_limit)
            rl_num = int(opts.rank_limit)
        else:
            sys.stdout.write(", no output rank limiting")
        
    else:
        sys.stdout.write(" one-best mbr")

    sys_weights = {}        
    sys_cost_bases={}
    if opts.sys_weight_str:
        sys_weights=parse_weights_announce("system_weight_string",opts.sys_weight_str)
    if opts.cost_bases_str:
        sys_cost_bases=parse_weights_announce("system-cost-base-string",opts.cost_bases_str)
        
    if False:    
        sys.stdout.write(", system_weight_string='%s'" % opts.sys_weight_str)
        wt_arr = opts.sys_weight_str.split(',')
        for wtspec in wt_arr:
            sysname,syswt = wtspec.split(':')
            sys.stdout.write(" %s:%f" % (sysname, float(syswt)))
            sys_weights[sysname] = float(syswt)
            
    sys.stdout.write("\n")
       
    systems = []
    files = []
    for arg in args:
        system, filename = arg.split("=")
        sys.stdout.write("System %s in file %s\n" % (system, filename))
        systems.append(system)
        files.append(open(filename))
        
    num_sys = len(systems)
    more = True

    next_lines = []
    sent_no = 0
    while more:
        lines = []
        recycle_lines = next_lines
        next_lines = []
        sent_no += 1
        if opts.max_sents and opts.max_sents<sent_no:
            break
        for sysno in xrange(0,num_sys):
            got_nbest = False
            if opts.nbest:
                while not got_nbest:
                    if len(recycle_lines) > 0:
                        line = recycle_lines.pop(0)
                    else:
                        line = files[sysno].readline()
                        if line == "":
                            more = False
                            break
                        line = systems[sysno] + " " + line.strip()
                    line_sent_no = int(line.split()[2])  # sent # is pos 2
                    if line_sent_no > sent_no:
                        next_lines.append(line)
                        got_nbest = True
                    elif line_sent_no < sent_no:
                        sys.stderr.write("ERROR - found line out of order: %s\n" % line)
                    else:
                        if (len(line.split()) > 6):  # ignore empty hyps
                            lines.append(line)                
            else:
                line = files[sysno].readline()
                if line == "":
                    more = False
                    break
                lines.append(systems[sysno] + " " + line.strip())

#        if len(lines) == 0:
#            break

        if len(lines) == 0:
            sys.stdout.write("\n")
        else:
            best_list,bleus = mbr_best(lines, opts.nbest, opts.cost_weighting, rl_num, rw_num, sys_weights, sys_cost_bases=sys_cost_bases,fast=opts.fast,addprec=opts.add_prec,eff_ref_len=opts.eff_ref_len,n=4,cost_base=opts.cost_base,normalize_cost_base=opts.normalize_cost_base,per_system_norm=opts.per_system_norm)
            sys.stdout.write(" ||| ".join([lines[b] for b in best_list]) + "\n")
            
    for f in files:
        f.close()
