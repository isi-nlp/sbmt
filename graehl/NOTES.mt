postedit+viz
tree lm
local features (esp. using f word-based distributions)
full pipeline

promise(0-10) difficulty(0-10) novelty(Old,New,?) idea

MT ideas:

3 2 N? mert search dir by taking local approx to corpus-bleu gradient
5 3 O C++ forest-mert for isi system (david says his python was too slow)
7 8 N rule probs (or independently factored features) conditioned on all already available state, with backoffs.  e.g. the source words (esp. just outside).  for ngram integrated, e.g. for "V -> x0 f"  "p(f|ngram-state(x0))".  huge space of possibilities.  e.g. child/result length distributions (already done by david+michael).  extension of length: (i,j) out of n for sentence 0..n, binned or continuous dist.
3 4 ? variations of CLM (CLM barely worked) using word classes, predicting or conditioning synt. category as well as E,F words, etc.  note CLM is just a variant of the general idea for rule probs+features above - it's state you have when decoding a single f string with an ngram e lm.
6 8 O large tuning set (to support many features really) - engineering challenge, needs actual useful features.
6 9 ? more reasonable segment+align for language pairs with differing inflectedness.  GIZA++ is bad at N-1 1-N.  GIZA-like that knows about stems vs. inflections?
3 3 ? fsa-lm-like or not long-distance features like agreement (can't do fsa-lm if you have to distinguish tree structures w/ same yield)
3 6 ? language pairs with good parsers on both sides (e.g. chi/en?) - empirically, which tree structure gives better MT for a given translation direction (as long as we don't have fancy custom target tree structure features).  only difficulty: involves full pipeline
4 4 O SAMT for ISI syntax decoder (david already did something SAMT-inspired for hiero; steve's STIG is SAMT-like)
5 5 O more generally: any way (SAMT or not) of getting non-const. phrasal rules in so they help (prior work: Steve)
5 5 O? GHKM from forest+lattice - find a good parser w/ meaningful parse forests, lattice from STT, morph/seg etc.
4 4 O? from web or part of potential LM training: x-lang IR on source doc,para,sent to find docs to train or bias a target LM
3 3 O? soft wordlcass lm, or hard wordclass for digit-strings
3 7 O bayesian GHKM that forgets original alignments - seems too slow for now

decoder speedup ideas:

4 6 N? >1 stateful-model nested cube pruning - apply models 1 at a time cube-style, not using a N-features as 1-feature shim
6 6 N l2r fsa intersection
4 6 O bottom up fsa intersection (zhang+gildea tried, weak compared to cube pruning)
2 2 N remove redundant composed rules (worse prob than components).  speedup only
5 4 O multipass by refining edges directly, or remembering outside for NT+span+full-state, with various ngram n and wordclassing (#bits per word) refinements - Petrov
5 5 O multipass by repeated refinement of TM grammar NTs - Petrov, others

techniques/tools:

??? is it possible to upper-bound the impact of features (early test to discard useless ideas) in some reranking framework?  how would this work with tuning's fear/hope decoding?  problem is: if it helps significantly as a nbest reranking feature, you know it's good.  if it doesn't help, you think: "maybe it will work when integrated" and waste time implementing it

system combination? (many people think this is not real MT research)

4 5 O self-combination - subsampled training subsets, diff training run weight vectors, then use usual syscom method on result - potential bayesian-like integration?

=====================

cdec (esp. hiero grammar) - learn+use hiero rules over larger spans (possibly use > learn?)

===

decoder idea: lazily nested cube trees.  i.e. group together all items sharing same model0...modeli context, and keep just list of (score,hyperedge,model{i+1} state) inside it.  probably not worthwhile space savings, but you could do early stopping of actually computing model scores.  also, some models could have their own approximate, greedy, or cube crossproduct (for binary rules)

===

p(compositions) model, not composed rules.  backoff to SAMT nonterminal or automatically learned classes.  state = ruleid, messy.

===

(done @isi) smoothed source length distribution for rules
todo: also predict left, right relative positions
todo: also outside source words?

===

binary rule x0 f or f x0 ... p(f|e_lr(x0)).  binary rule f or f f2 ... p(f|unary) p(f f2|binary) ?

===

ngram agreement model (skip featureless intervening words) e.g. plural/singular

===

giza-align stemmed words, translated inflected

===

rule acq. from forest+lattice (parser ambig, lattice=stt,delete useless foreign words, morph )


===

p(x|f,a,b)=p(x|f_{a-1},f_{b+1})

===

source-side parse tree:string ghkm; extract in reverse direction, then reverse rules

===

x-lang ir lm - doc or para. level.  eval issue: webcrawl in advance up to cutoff date (is this why google doesn't do evals?)

===

tunable indep. features for soft-wordclass lms?

===

syscom by decoding system N w/ weighted from previous N-1's nbests/forests "in high score translation" ngram lm (rewarding overlap in search).  decrease in independent system vote effect?

===

discriminative testing of LM ideas (e.g. different hard/soft number or other entity wordlcassing) based on hope/fear nbests from tuning?  (i.e. test them as reranking features, to save running pipeline to get integrated-search impact)

infrastructure for really reproducible experiments; by trying many configurations, we overfit our "held out" data in research.  the knowledge to reproduce old experiments is gone.  all experiments must be reproducible and can be evaluated on further blind data automatically when it becomes available.  (paired?) significance testing?  impact of just one feature?  explicit knowledge of what features are in baseline, etc.  obviously requires strict versioning - published pathname refers to immutable data, or else, exact configuration info necessary to build the object used.

========
I asked if we use digit->@ rewriting for lm training and scoring.  Michael tells me we do.  This means we have to use the same convention in training dependency and cross-language LMs (we store only the words post-@ and <unk> as remembered by the LM, not the original words; we could change this but it would make ngram decoding slightly less efficient).

Someday I want to investigate either:

A) alternative fixed number->wordclass mapping rules (e.g. leave 0-31 alone, 202@ 2019 ... 2010 2009 ... 2000 2001 ... 1999 ... 1990 198@ 197@ ... 18@@ for yearlike-numbers)

B) soft wordclassing, i.e. put each word at a leaf in a tree, with classes as internal nodes, with the <unk> backoff at the root. In order to use existing LM data structures (like the nice noisy biglm hash), I'd want to specify a simple recipe for performing a series of wordclass-unaware queries, e.g. the backoffs would be tried in a fixed sequence until a match is found, for instance, if i have a specific bigram "1993 X" but haven't seen the trigram "fixed 1993 X", I don't try to backoff to "fixed 199@ X".  Training would have to model a cost for generalizing the predicted word, but not generalizing the contexts.

It's probably not been done yet (by us) because:

1) we use huge language models, and retraining is annoying.

2) the decoder's interpretation of the language model must follow the new scheme.

======
tuning for search-error-affecting switches.  annotate nbests w/ options used and rank in nbest?  extend to forests?

====
replace old feature w/ "improved" one, or add both - tuning method that can make the right decision (priors, correlated features)?

