#ifndef graehl__sblm_info_hpp__
#define graehl__sblm_info_hpp__
#define SBLM_EXTRA_DEBUG 0

#if SBLM_EXTRA_DEBUG
# include <graehl/shared/myassert.h>
#ifndef GRAEHL_ASSERT
# define GRAEHL_ASSERT 1
#endif
# define SBLM_FINITE(x,n) Assert(!nonfinite_cost(x,n));
#else
# define SBLM_FINITE(x,n)
#endif

#define USE_SMALL_VECTOR 1

//#include <boost/noncopyable.hpp>
#include <graehl/shared/funcs.hpp>
#include <graehl/shared/char_is.hpp>
#include <sbmt/hash/oa_hashtable.hpp>
#include <graehl/shared/dynamic_array.hpp>
#include <graehl/shared/null_output_iterator.hpp>
#include <sbmt/logging.hpp>
#include <sbmt/edge/scored_info_array.hpp>
#include <sbmt/ngram/dynamic_ngram_lm.hpp>
#include <sbmt/edge/any_info.hpp>
#include <sbmt/edge/info_base.hpp>
#include <sbmt/span.hpp>
#include <sbmt/hash/oa_hashtable.hpp>
#include <gusc/generator/single_value_generator.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/function_output_iterator.hpp>
#include <sbmt/range_io.hpp>
#include <sbmt/grammar/lm_string.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/grammar/syntax_rule.hpp>
#include <sbmt/feature/indicator.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/small_vector.hpp>

//#if !defined(SBLM_FEATNAME)
# define SBLM_FEATNAME "sblm"
# define SBLM_BIFEATPRE "sb"
# define SBLM_PCFEATPRE "spc"
//#endif

# define SBLM_DEBUG_PRINT_RULE 0
// print syntax rule too

namespace sblm {


static char const*const sblm_featname=SBLM_FEATNAME;
static char const*const sblm_unkword_featname=SBLM_FEATNAME "-unkword"; // unknown (to sblm) terminals
static char const*const sblm_unkcat_featname=SBLM_FEATNAME "-unkcat"; // unknown nonterminals (+preterms)
static char const*const sblm_pword_featname=SBLM_FEATNAME "-pword"; // logp(word|tag)

static char const*const sblm_nts_featname=SBLM_FEATNAME "-nts";
static char const*const sblm_bifeatpre=SBLM_BIFEATPRE;
static char const*const sblm_pcfeatpre=SBLM_PCFEATPRE;

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(sblm_domain,sblm_featname,sbmt::root_domain);

//TODO: enable greedy-unscoreable-only (decoder should support - also a speedup for full greedy)
extern bool greedy; // make sure you link in such a way that there's only one inclusion of sblm_info.hpp anywhere that matters

using namespace sbmt;
using namespace sbmt::io;
using namespace std;
using namespace boost;
using namespace graehl;

typedef dynamic_ngram_lm LM;
typedef LM::lm_id_type lmid;
typedef dynamic_array<lmid> lmids;
# if USE_SMALL_VECTOR
typedef graehl::small_vector<lmid,2> info_lmids;
# else
typedef lmids info_lmids;
# endif
typedef lmids::iterator lmidp;
typedef lmids::const_iterator lmidcp;
typedef grammar_in_mem G;
typedef indexed_syntax_rule R;
typedef R::tree_node rtree;
typedef std::pair<boost::uint32_t,score_t> component_score;
typedef lmids pcfg_rewrite;
typedef boost::uint32_t fid_t;
template <class G>
inline double get_wt(G const& g,char const* featname) {
  return get(g.get_weights(),g.feature_names(),featname);
}
template <class G>
inline fid_t get_fid(G &g,char const* featname) { // nonconst because we add feat name to dict if needed
  return g.feature_names().get_index(featname);
}


const unsigned rhs_index_max=1<<16;

// all this to avoid typed union / variant (rhs index for -bar vars vs. lm id)
inline bool is_skip(lmid i) {
  return i==(lmid)LM::null_id;
}

//approx: !is_index && !skip
inline bool is_normal(lmid i,lmid index_origin,lmid unk_id) {
  return i<index_origin || i==unk_id;
}

inline bool is_index(lmid i,lmid index_origin) {
  return i>=index_origin && i<(index_origin+rhs_index_max); // lmid is unsigned so don't subtract.
}

inline unsigned index(lmid i,lmid index_origin) {
  //assert(is_index(i,index_origin));
  return i-index_origin;
}

inline void print_lmid(std::ostream&o,lmid i,lmid origin,LM const& lm) {
    if (is_skip(i))
      o<<"[skip]";
    else if (is_index(i,origin)) {
      o<<'['<<index(i,origin)<<']';
    } else
      o<<lm.word(i);
}

template<class C>
void print_rewrite(std::ostream&o,C const& r,lmid origin,LM const& lm) {
  for(unsigned i=0;i<r.size();++i) {
    if (i==1)
      o<<" -> ";
    else if (i>0)
      o<<" ";
    print_lmid(o,r[i],origin,lm);
  }
}


struct sblm_string;
struct debug {
  LM const* lm;
  G *g;
  lmid origin;
  debug() : lm(),g() {  }
  void print(syntax_id_type syntax_id,ostream &o=cerr) {
    if (g)
      g->print_syntax_rule_by_id(o,syntax_id,false);
    else
      o<<"(syntax-id-for-unknown-grammar)"<<syntax_id;
  }
  void print_lm(lmid id,ostream &o=cerr) {
    if (lm)
      print_lmid(o,id,origin,*lm);
    else
      o<<"(sblm-lmid-for-unknown-LM)"<<id;
  }
  template <class C>
  void print(C const& r,ostream &o=cerr) {
    if (lm)
      print_rewrite(o,r,origin,*lm);
    else
      o<<"(sblm-rewrite-for-unknown-LM)"<<r;
  }
  void print(sblm_string const& s,ostream &o=cerr);

  static debug dbg;
};

struct inside_accum
{
    score_t *pi;
    explicit inside_accum(score_t &inside) :pi(&inside) {}
    inline void sequence_prob(LM const&lm,lmid const*p,lmid const*sep,lmid const*end) const
    {
        *pi *= lm.sequence_prob(p,sep,end);
    }
};

// inplace: only operate on threadlocal (stack) copies!

// *ctx_start should be <s> or whatever; score (ctx_start ... [start ... end-3,p,end-2]) and all smaller ngrams down to final (ctx_start ... [p start]). i.e. [ctx_start,end) has end[-1] as scratch space at first. end[-2] is the </s> or whatever
// destructive.
template <class A>
void pcfg_score_complete_inplace(LM const& lm,lmid p,lmidp ctx_start,lmidp start,lmidp end,A const&a) {
  --end;
  for (;end>start;--end) {
    lmidp n=end-1; // final iteration: n=start,end=start+1
    // suppose p=L [ctx_start...end] was = <s> A B * - then, first iteration:
    *end=*n; // <s> A B =B
    *n=p;  // <s> A =L B
    a.sequence_prob(lm,ctx_start,end,end+1); // score B | <s> A , p=L
  }
}

# if 0
// pc should be lm.end_id terminated already
template <class A>
void pcfg_score_complete_inplace(LM const& lm,pcfg_rewrite &pc,A const&a) {
  lmidp start=&pc[0]; // *L -> A B (e.g.)
  lmid p=*start; // L
  *start=lm.start_id; // =<s> A B *
#if 1
  pcfg_score_complete_inplace(lm,p,start,start+1,pc.end()-1,a);
#else
  for (lmidp c=pc.end()-1;c>start;--c) {
    *c=c[-1]; // <s> A B B
    c[-1]=p;  // <s> A L B
    a.sequence_prob(lm,start,c,c+1); // score B | <s> A , p=L
  }
#endif
}
#endif

template <class A>
void pcfg_score_complete(LM const& lm,pcfg_rewrite const& pc,A const&a) {
#if 0
              SBMT_DEBUG_MSG(sblm_domain,"e-context map: %6% ngram-id=%2% ngram-word=%3% %1%-id=%4% %1%-word=%5%",name%oid%other.word_raw(oid)%myid%me.word_raw(myid)%l);
      SBMT_PEDANTIC_EXPR(sblm_domain,
                       {
                           continue_log(str) << "pcfg_score_complete";
                       });
#endif
  pcfg_rewrite c(pc,0,2); // 2 extra elements after. (0 before)
  lmid p=c[0]; // L
  c[0]=lm.start_id;
  c[pc.size()]=lm.end_id; // now c = <s> R1 ... Rn </s> _ ; p will be ruffled through ending with final prob <s> L ... Rn p </s>
  lmidp b=c.begin(); // *<s> R1 ...
  pcfg_score_complete_inplace(lm,p,b,b+1,c.end(),a);
}

// contains some gaps (null_id) which we skip
//FIXME: test or prove mightily.
//returns true if the score is heuristic (skipped_parent or null parent or any variables). otherwise it can be applied to inside
inline bool pcfg_score(LM const& lm,lmid origin,pcfg_rewrite const& pc,score_t &s,bool skipped_parent)
{
  lmid p=pc[0];
  if (is_skip(p)) { // see token_to_sblm constructor ; only under some settings will a skip (none) lmid be assigned to a -BAR parent; we will know the parent if unsplitting happens and splitting was after binarization in training.
    lmid unk=lm.unk_id;
    for (lmidcp i=pc.begin()+1,e=pc.end();i!=e;++i)
      if (is_normal(*i,origin,unk))
        s*=lm.prob(*i);
    return true;
  }
  unsigned noskip=!skipped_parent;
  pcfg_rewrite c(pc,0,noskip+1); // 1 extra element for <s>, 1 for NULL
  if (noskip) {
    c[0]=lm.start_id;
    c[pc.size()]=lm.end_id;
  }
  SBMT_DEBUG_EXPR(sblm_domain,{
      debug::dbg.print_lm(p,continue_log(str)<<"pcfg_score: p=");
      debug::dbg.print(c,continue_log(str)<<" ; ");
    });
  bool skipped=skipped_parent;
  lmidp i=c.begin()+1,end=c.end()-1; // we'll score nd has 1 space extra so we can use i[1] below:
  lmidp ctx_start=noskip ? c.begin() : i;
  for (;i<end;++i) {
    if (is_index(*i,origin)) {
      ctx_start=i+1;
      //start=i;
      skipped=true;
    } else {
      // i will be scored. place parent in its place and i in i+1's (then restore)
      // first iter for L -> A B with noskip:
      lmid nn=i[1]; // <s> (@i)A nn=B </s>
      SBMT_PEDANTIC_EXPR(sblm_domain,{
          debug::dbg.print_lm(*i,continue_log(str)<<"pcfg_score for word "<<" @i="<<i-c.begin()<<" lmid="<<*i<<": ");
        });
      i[1]=i[0];    // <s> A A </s>
      i[0]=p;       // lm.prob[ <s> L A ) </s>
      score_t sp=lm.prob(ctx_start,i+2); // scores word i[1]
      s*=sp;
      i[0]=i[1];    // <s> A A </s>
      i[1]=nn;      // <s> A B </s>
      // (it's possible we can increase efficiency by using another local temp, or scoring right to left - but this way we know where the context starts (left->right))
    }
  }
  return skipped;
}

inline score_t pcfg_bar_h_score(LM const& lm,lmid origin,pcfg_rewrite const& pc) {
  score_t h;
  bool ish=pcfg_score(lm,origin,pc,h,true);
  (void)ish;
  assert(ish);
  return h;
}

inline score_t pcfg_h_score(LM const& lm,lmid origin,pcfg_rewrite const& pc,bool skipped_root) {
  return skipped_root?pcfg_bar_h_score(lm,origin,pc):lm.prob(pc[0]);
}

template <class A>
void pcfg_bigram_indicators(LM const& lm,pcfg_rewrite const& c,A & a) {
  if (c.size()<2) {
    assert(!c.empty());
    a(make_tuple(lm.start_id,lm.end_id),sbmt::indicator);
    return;
  }
  a(make_tuple(lm.start_id,c[1]),sbmt::indicator);
  unsigned e=c.size()-1;
  for (unsigned i=1;i<e;++i)
    a(make_tuple(c[i],c[i+1]),sbmt::indicator);
  a(make_tuple(c[e],lm.end_id),sbmt::indicator);
}

template <class A>
void pcfg_pcbigram_indicators(LM const& lm,pcfg_rewrite const& c,A & a) {
  unsigned e=c.size();
  lmid p=c[0];
  for (unsigned i=1;i<e;++i)
    a(make_tuple(p,c[i]),sbmt::indicator);
}

//handle preferences for figuring out which sblm labels go with which xrs tree labels (skip -bar, remove -1 -2)
struct treebank_categories {
  bool skip_bar;
  bool do_unsplit;
  bool split_after_bar;
  bool terminals;
  bool indicator_terminals; // always false! <s> word and word </s> are boring.
  bool pword;
  void print(std::ostream &o) const {
    o<<"skip-bar="<<skip_bar<<" [NP(@NP-BAR(A B) C)->NP(A B C)] ; unsplit="<<do_unsplit<<" [NP-2 -> NP] ; ";
    o<<"score-terminals="<<terminals;
  }
  friend std::ostream &operator<<(std::ostream &o,treebank_categories const& t) {
    t.print(o);
    return o;
  }

  treebank_categories() {
    set_defaults();
  }
  void set_defaults() {
    pword=true;
    skip_bar=true;
    do_unsplit=true;
    split_after_bar=true; // this means if you don't unsplit, then bar parents could belong to any subcat. always true.
    terminals=true;
    indicator_terminals=false; // always false
  }
  bool skip(string const& s) const {
    return skip_bar && s.size() && s[0]=='@';
  }
  string unskip(string const& s) const {
    assert(skip(s));
    //assert(match_end(s,"-BAR"));
    string::const_iterator n=s.end();
    bool barend=s.size()>4 && (n[-4]=='-'&&n[-3]=='B'&&n[-2]=='A'&&n[-1]=='R');
    return unsplit(string(s.begin()+1,
                          barend?s.end()-4:s.end()));
  }
  string unsplit(string const& s) const { // call on treebank NTs only for now
    if (!do_unsplit) return s;
    //assert(skip_bar); //FIXME: if !skip_bar, need to find the -NN before -BAR and delete it
    string::size_type minus=s.rfind('-');
    if (minus==string::npos || !graehl::all(s.begin()+minus+1,s.end(),graehl::pisdigit())) return s;
    return string(s.begin(),s.begin()+minus);
  }
  bool is_identity() const {
    return !skip_bar && !do_unsplit;
  }
  bool needs_words() const {
    return terminals || pword;
  }
  bool skipped_root_is_incomplete() const {
    return split_after_bar && skip_bar && !do_unsplit;
  }
  template <class O>
  void add_options(O &opts) {
        opts.add_option(SBLM_FEATNAME "-skip-bar",
                    optvar(skip_bar),
                    "skip over restructured @...-BAR nodes, flattening children until non-@ (treebank) NT");
        opts.add_option(SBLM_FEATNAME "-unsplit",
                        optvar(do_unsplit),
                        "normalize CAT-... e.g. NP-1 and NP-2 to CAT e.g. NP; note: don't enable this if your categories aren't split; whatever follows the last - is removed");
        opts.add_option(SBLM_FEATNAME "-split-after-bar",
                        optvar(split_after_bar),
                        "if category splitting happened before restructuring, then skip-bar means you know the parent. if splitting happens after, then only if you unsplit do you know the parent. if there is no category splitting at all, then set both unsplit and split-after-bar to false");
        opts.add_option(SBLM_FEATNAME "-terminals",
                        optvar(terminals),
                        "include preterminal->terminal rewrite events in sblm score");
        opts.add_option(SBLM_FEATNAME "-pword",
                        optvar(pword),
                        "output separate sblm-pword logprob feature for preterminal->terminal rewrite events in sblm score");
/* //silly option: words are always <s>word</s> because preterminals aren't optional.
   opts.add_option(SBLM_FEATNAME "-indicator-terminals",
                        optvar(do_unsplit),
                        "include preterminal->terminal rewrite events in sblm score");
*/
  }
};

//////

/*
  context for scoring rules including token -> lmid maps, and packing of variable indices into unused lmids. (variable at i=index-index_origin if that's 0<=i<rhs_index_max. in practice this means any lmid that isn't null or unk, and is >= index_origin.
 */

inline string quote_blind(string const& s) {
  return "\""+s+"\"";
}

struct token_to_sblm { // maps native tags to simplified (remove -BAR if skipping). native tokens are mapped by quote_blind into wordmap
  typedef dynamic_array<lmid> idmap_t; // index to ngram id
  idmap_t tagmap,wordmap,skipmap; // both english words and categories map to ngram ids. for cats, null_id if it's a skipped node
  LM const* plm;
  LM const& lm() const { return *plm; }
  lmid index_origin;
  lmid top_lmid;
  lmid unk_id;
  treebank_categories tbc;
  bool terminals;
  bool words;
  token_to_sblm(treebank_categories const&tbc,indexed_token_factory &dict,LM const& lm) : tbc(tbc),terminals(tbc.terminals),words(tbc.needs_words()) {
    debug::dbg.origin=index_origin=lm.first_unused_id();
    plm=&lm;
    unk_id=lm.unk_id;
    if (words) {
      BOOST_FOREACH(indexed_token t, dict.native_words()) {
        at_expand(wordmap,t.index()) = lm.id_existing(quote_blind(dict.label(t))); // may have digit->@
      }
    }
    top_lmid=lm.id_raw_existing(top_token_text); // TOP is never split/skipped
    BOOST_FOREACH(indexed_token t, dict.tags()) {
      lmid r=(lmid)LM::null_id,s=(lmid)LM::null_id;
      std::string const& tstr=dict.label(t);
      unsigned i=t.index();
      if (tbc.skip(tstr)) {
        if (!tbc.skipped_root_is_incomplete()) {
          s=lm.id_raw_existing(tbc.unskip(tstr));
          assert(!is_skip(s));
        }
      } else
        r=lm.id_raw_existing(tbc.unsplit(tstr));
      at_expand(tagmap,i) = r;
      at_expand(skipmap,i) = s; //FIXME: array of tuple<bool,lmid> instead since one or other is always null?
    }
  }

  //TODO: change indexing into arrays so top token has index of 0 and others have index 1+tag.index(); then only one check for top needs be done
  static inline bool is_top(unsigned i) {
    return i==indexed_token::top_index; //FIXME: ugly impl detail: no token may have this index except one of top type. could check type of tokens and make index-methods private instead.
  }

  lmid skip_id(unsigned i) const {
//    return is_top(i) ? (lmid)LM::null_id : skipmap.at_assert(i);
    assert(!is_top(i));
    return skipmap.at_assert(i);
  }
  lmid skip_id(indexed_token t) const {
    assert(is_native_tag(t));
    return skip_id(t.index());
  }

  lmid lhs_id(unsigned i) const {
    return is_top(i) ? top_lmid : tagmap.at_assert(i);
  }
  lmid lhs_id(indexed_token t) const {
    assert(is_native_tag(t));
    return lhs_id(t.index());
  }

  bool lhs_skipped(unsigned i) const {
    return is_top(i) ? false : is_skip(tagmap.at_assert(i));
  }
  bool lhs_skipped(indexed_token t) const {
    assert(is_native_tag(t));
    return lhs_skipped(t.index());
  }

  // returns null_id if the tag is a -bar (skipped) one
  lmid id(indexed_token t) const {
    if (is_native(t)) {
//      assert(terminals);
      return words ? wordmap.at_assert(t.index()) : unk_id;
    }
    return lhs_id(t);
  }
};

inline size_t hash_lmids(info_lmids const& l) { // use hash_value from info_lmids' namespace (name conflict with info_base hash_value member)
  return hash_value(l);
}

struct sblm_info : info_base<sblm_info> {
  enum { null_id=dynamic_ngram_lm::null_id };
  info_lmids cats; // treebank nts and preterms (skip @...-bar), collapsed without catsplits e.g. NP-2 => NP
  typedef grammar_in_mem G;
  //sblm_info() {}
//  sblm_info(lmid root) : vars(1,lmids(1,root))  {  } // we store this redundant category (same as syntax rule root, chart tag) to simplify implementation for now. later use an in-place optimized vector if we want. would need to look up indexed token -> lmid map also
  bool equal_to(sblm_info const& other) const
  {
    return sblm::greedy || cats==other.cats;
  }
  size_t hash_value() const { return sblm::greedy ? 0 : hash_lmids(cats); }
};

//typedef sblm_info const* CP;
typedef scored_info_array<sblm_info> child_ps;

struct sblm_string {
  score_t inside,total,wordp; // already computed scores. total is used as heuristic since we never return a true inside score to the grammar. separate pword feature is optional
  typedef dynamic_array<pcfg_rewrite> pcfg_rewrites;
  pcfg_rewrites events; // some of these are complete (no var indices). others are not. TODO: easy change to consider non-skip variables e.g. x0:NP as variables even though for a simple PCFG they could just be leaves. will extend to stateful e.g. grandparent/headed more elegantly that way. if lmid>origin then lmid-origin is the rhs index.
  bool skipped_root; // if root of whole rule is a skip node (means we compute a state of list of children)
//  enum { incomplete=0,terminal=1,nonterminal=2 };
  //terminal = leaf in rule which is not a variable (must be native word, it turns out). option to skip terminal events entirely. result item may be called 'nonterminal' too; incomplete means there are variable indices corresponding to -BAR (skipped) nodes.
  typedef char event_status;
  static const event_status incomplete = 0;
  static const event_status terminal = 1;
  static const event_status nonterminal = 2;
  dynamic_array<event_status> status; // from enum above
  token_to_sblm const& tokens;
  LM const& lm;  // both these refs should remain valid because factory will still exist, and we use smart ptrs in factory so we don't even have to rely on non-copying after type erasure
  lmid unk_id;
  //R const& rule;
  lmid origin;
  bool terminals;
  template <class O>
  void print(O &o,lmid i) const {
    print_lmid(o,i,origin,lm);
  }
  template <class O>
  void print(O &o,pcfg_rewrite const& r) const {
    print_rewrite(o,r,origin,lm);
  }
  template <class O>
  void print(O &o) const {
    o<<"sblm_string[id="<<syntax_id<<"]={{{";
    o<<"\n\tinside="<<inside<<" h="<<total/inside;
    if (complete_result())
      o<<" complete_result_h="<<complete_result_h;
    else
      o<<" (incomplete result)";
    o<<"\n\tskipped_root="<<skipped_root<<" nts="<<nts;
    if (unkwords) o<<" unkwords="<<unkwords;
    if (unkcats) o<<" unkcats="<<unkcats;
    assert(status.size()==events.size());
    for (unsigned i=0;i<events.size();++i) {
      o<<"\n\t"<<status_str(status[i]);
      print(o,events[i]);
    }
    if (SBLM_DEBUG_PRINT_RULE) {
      o<<"\n\t";
      debug::dbg.print(syntax_id,o);
    }
    o<<"\n}}}";
  }
  void dump() const {
    print(cerr);
  }
  sblm_string(token_to_sblm const& tokens,lmid origin) : tokens(tokens),lm(tokens.lm()),origin(origin),terminals(tokens.terminals) {
    unk_id=lm.unk_id;
  }
  // result()[0] (parent) => resulting info state for simplicity
  pcfg_rewrite const& result() const {
    //assert(is_complete());
    return events[0];
  }
  score_t complete_result_h; // in the case where the result is known no matter what variables plug in, we can precompute the heuristic for the resulting info state here.
  void cache_complete_result_h() {
    if (complete_result())
      complete_result_h=h_score(result());
  }
  score_t h_score(pcfg_rewrite const& completed) const {
    return pcfg_h_score(lm,origin,completed,skipped_root);
  }
  char const* status_str(event_status s) const {
    switch(s) {
    case incomplete: return "(partial)";
    case terminal: return "(terminal)";
    default: return "";
    }
  }
  bool complete_result() const { return status[0]!=incomplete; }
private:
  unsigned n_events;
  unsigned unkwords,unkcats,nts;
  typedef vector<indexed_syntax_rule::rule_offset_t> rhsi2vari_impl;
  typedef rhsi2vari_impl const& rhsi2vari;
  //temporaries for tree recursion
public:
  syntax_id_type syntax_id;
  // init not called in ctor so we can save on a copy in push_back (stupid optimization)
  void init(R const& rule) {
    nts=unkwords=unkcats=0;
    syntax_id=rule.id();
    rtree const& t=*rule.lhs_root();
    unsigned maxrw=rule.lhs_size();
    events.resize(maxrw); // preallocating max possible means we don't need to constantly reindex (can hold iterators) while appending at various levels in tree recursion
    status.reinit(maxrw,nonterminal);
    n_events=1; // can never have fewer events than this (no x0 -> x1 rules allowed; there's always a root R(x0:S) -> x1)
    assert(is_native_tag(t.get_token()));
    unsigned ti=t.get_token().index();
    skipped_root=tokens.lhs_skipped(ti);
    lmid rooti=skipped_root?tokens.skip_id(ti):tokens.lhs_id(ti);

    //TODO: refactor to use descend_root instead?
    if (!skipped_root) ++nts;
    if (rooti==unk_id) ++unkcats;
    events[0].push_back(rooti);
    //note: result isn't incomplete just because we have unscored items because lhs is a -BAR (skipped)
    rhsi2vari_impl vari=rhs_to_vars_only_index(rule);
    descend_root_children(t,vari,0);

    events.resize(n_events);
    status.resize(n_events);
  }
  // set comp=false iff there's a -bar variable in rewrite w
  void descend_root(rtree const& t,lmid ti,rhsi2vari vari,unsigned ei) {
    if (ti==unk_id) ++unkcats;
    ++nts;
    events[ei].push_back(ti);
    descend_root_children(t,vari,ei);
  }
  void descend_root_children(rtree const& t,rhsi2vari vari,unsigned ei) {
    for (R::lhs_children_iterator i=t.children_begin(),endc=t.children_end();i!=endc;++i)
      descend_past_skips(*i,vari,ei);
  }
  void descend_past_skips(rtree const& t,rhsi2vari vari,unsigned ei) {
    lmid ti=tokens.id(t.get_token());
    bool skipped=is_skip(ti);
    pcfg_rewrite &w=events[ei];
    bool var=t.indexed();
    assert(!var || is_native_tag(t.get_token()));
    if (skipped) {
      if (var) { // variable with -BAR node
//TODO: change condition to just t.indexed() once e.g. grandparent dep.
        assert((unsigned)t.index()<vari.size());
        assert(vari[t.index()]!=(indexed_syntax_rule::rule_offset_t)-1);
        w.push_back(origin+vari[t.index()]); // store var index of var
        status[ei]=incomplete;
      } else
        for (R::lhs_children_iterator i=t.children_begin(),endc=t.children_end();i!=endc;++i)
          descend_past_skips(*i,vari,ei);
    } else { // have a non-skipped tag or terminal to append
      w.push_back(ti);
      if (!t.is_leaf()) { // internal node: recurse
        if (ti==unk_id) ++unkcats;
        assert(is_native_tag(t.get_token())); // can't be terminal
        ++n_events;
        descend_root(t,ti,vari,n_events-1);
      } else {
        if (!var) {
          status[ei]=terminal; // a rewrite with ANY terminal will be skipped if that option is enabled. fortunately we always have preterminals for each terminal
          if (ti==unk_id) ++unkwords; // TODO: don't trigger feat unless terminals - for now sanity check sblm-unkword = text-length (except counting lm-unk also)
          assert(is_native(t.get_token()));
          assert(terminals || ti==unk_id);
        }
      }
    }
  }
  void score() {
    score_t h;//=1
    inside=1; // must be idempotent! because we rescore rule during compute_info output if PEDANTIC logging level
    wordp=1;
    SBMT_PEDANTIC_EXPR(sblm_domain,{
        debug::dbg.print(syntax_id,continue_log(str)<<"score rule: ");
      });
    for (unsigned i=0;i<n_events;++i) {
      event_status s=status[i];
      pcfg_rewrite &rw=events[i];
      bool skipped_parent=(i==0&&skipped_root);
      score_t score;//=1
      bool ish=pcfg_score(lm,origin,rw,score,skipped_parent);
#ifndef SBLM_EXTRA_DEBUG
      bool goodh=ish == (s==incomplete || skipped_parent);
      if (!goodh) {
        cerr<<"ERROR: ish="<<ish<<" status="<<status_str(s)<<endl;
        debug::dbg.print(rw);
        cerr<<endl;
        debug::dbg.print(syntax_id);
        cerr<<endl;
      }
      assert(goodh);
#endif
      SBMT_DEBUG_EXPR(sblm_domain,{
          debug::dbg.print(rw,continue_log(str)<<"score id="<<syntax_id<<" ; rewrite: ");
          continue_log(str)<<" score="<<score<<" "<<(ish?"(heuristic)":"(inside)");
        });
      if (s==terminal) {
        assert(!ish);
        wordp*=score;
        if (terminals)
          inside*=score;
      } else
        (ish?h:inside)*=score;
    }
    total=inside*h;
    cache_complete_result_h();
    SBMT_DEBUG_EXPR(sblm_domain,{
        continue_log(str)<<"scored id="<<syntax_id<<" inside="<<inside<<" h="<<h;
      });
  }


  pcfg_rewrite const& complete(event_status s,pcfg_rewrite &c,pcfg_rewrite const& rw,child_ps const& cps) const {
    if (s==incomplete) {
      complete(c,rw,cps);
      return c;
    }
    return rw;
  }

  void complete(pcfg_rewrite &c,pcfg_rewrite const& rw,child_ps const& cps) const {
    c.clear();
    c.reserve(rw.size()*2+8);
    lmidcp i=rw.begin(),e=rw.end();
    assert(i!=e);
    c.push_back(*i);++i;
    for (;i!=e;++i) {
      lmid id=*i;
      if (is_normal(id,origin,unk_id)) //!is_index(id,origin))
        c.push_back(id);
      else {
        unsigned rhsi=index(id,origin);
        unsigned cpsn=cps.size();
        SBMT_DEBUG_EXPR(sblm_domain,{
            debug::dbg.print(rw,continue_log(str)<<"complete rhsi="<<rhsi<<" of "<<cpsn<<" rw=");
            debug::dbg.print(c,continue_log(str)<<" into c=");
            for (unsigned j=0;j<cpsn;++j) debug::dbg.print(cps[j]->cats,continue_log(str)<<"\n\tcps["<<j<<"]=");
          });
        assert(rhsi<cpsn);
        c.append(cps[rhsi]->cats);
      }
    }
  }

  template <class BigramAccum>
  void compute_info(child_ps const& cps,info_lmids &result_out,score_t &inside_out,score_t &h_out,BigramAccum &bigramaccum,bool use_bigram,BigramAccum &pcbigramaccum,bool use_pcbigram,bool debug,unsigned &unkwords_out,unsigned &unkcats_out,unsigned &nts_out,score_t &words_out) const {
    words_out=wordp;
    unkwords_out=unkwords;
    unkcats_out=unkcats;
    nts_out=nts;
    SBMT_PEDANTIC_EXPR(sblm_domain,{
        debug::dbg.print(*this,continue_log(str)<<"compute_info(cps.size="<<cps.size()<<" ");
      });
    if (debug && use_loglevel(sblm_domain)>=lvl_debug)
      const_cast<sblm_string &>(*this).score();
    assert(h_out.is_one());
    assert(result_out.empty());
    inside_out=inside; //TODO: make this affect rule inside scores in decoding (incl. cost pushing?) - better lazy cube ordering performance?
    inside_accum accum(inside_out); // all the complete rewrites except a possible skipped_root are scored already
    pcfg_rewrite completed_space; // repeatedly cleared
    for (unsigned i=0;i<n_events;++i) {
      event_status s=status[i];
      bool nonterm=(s!=terminal);
      //SUBSTITUTE -BAR VARIABLES' STATES
      pcfg_rewrite const&rw=complete(s,completed_space,events[i],cps);
      bool complete=s!=incomplete;
      //HEURISTIC
      if (i==0)
        h_out=complete?complete_result_h:h_score(rw);
      bool bar=(i==0&&skipped_root);
      //BIGRAMS
      if (nonterm && !bar) {
        if (use_bigram) pcfg_bigram_indicators(lm,rw,bigramaccum);
        if (use_pcbigram) pcfg_pcbigram_indicators(lm,rw,pcbigramaccum);
        // TODO (bar): emit internal bigrams immediately - requires addl complexity: need to not reemit when we complete (except the boundaries)
      }

      // NONEMPTY RESULT (because -BAR)
      if (bar)
        result_out.set(rw.begin()+1,rw.end()); // elseresult_out stays empty
      // ELSE SCORE if needed
      else if (s==incomplete && (terminals || nonterm))
        pcfg_score_complete(lm,rw,accum); // if complete, then we already included this score in this->inside. this->inside skipped terminals if necessary, of course
    }
  }
  //TODO: cache sblm_wt * inside,total - probably not a noticable speedup, and may want to change weight later? not likely; never heard of a weight changing once an info constructor happened, and we build these every time that happens.
};

template <class C, class T>
std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T>& o, sblm_string const& d) {
  d.print(o);
  return o;
}

inline void debug::print(sblm_string const& s,ostream &o) {
  o
# ifndef NDEBUG
    <<"@"<<hex<<&s<<" "<<dec
# endif
    <<s;
}

typedef tuple<lmid,lmid> bigram_feat_name;

inline size_t hash_value(bigram_feat_name const& a) {
  size_t x=hash_value(a.get<0>());
  hash_combine(x,a.get<1>());
  return x;
}

template <class C, class T>
void print(std::basic_ostream<C,T>& o, bigram_feat_name const& d, LM const& lm) {
  o<<sblm_bifeatpre<<'['<<lm.word(d.get<0>())<<"]["<<lm.word(d.get<1>())<<']';
}

struct sblm_strings {
  dynamic_array<sblm_string> contig_sblmstrs;
  lmid index_origin; // first variable has this value. subtract origin to get (rhs, skipping lex) variable number
  void init(G const& g,token_to_sblm const& tokens,LM const& lm) {
    G::syntax_range all=g.all_syntax_rules();

    index_origin=tokens.index_origin;
    //lm.first_unused_id();
    contig_sblmstrs.clear();
    contig_sblmstrs.reserve(10000000); //TODO: check that (pod) copy/swap works for sblm_string
    for (G::syntax_iterator i=all.begin(),e=all.end();i!=e;++i) {
      scored_syntax_ptr p=*i;
      p->set_contig_id(contig_sblmstrs.size());
      contig_sblmstrs.push_back(sblm_string(tokens,index_origin));
      sblm_string &s=contig_sblmstrs.back();
      s.init(p->rule); // push_back, ctor call, hope copy optimized out. or don't care about cost :)
      s.score();
    }
    SBMT_DEBUG_EXPR(sblm_domain,{
        continue_log(str)<<"ALL SBLM STRINGS:\n";
    for (G::syntax_iterator i=all.begin(),e=all.end();i!=e;++i) {
      scored_syntax_ptr p=*i;
      syntax_id_type id=p->syntax_id();
      continue_log(str)<<"\t\nsyntax_id="<<id<<" contig_id="<<p->contig_id<<" ";
      debug::dbg.print(id,continue_log(str));
      continue_log(str)<<"\t\n@"<<hex<<p<<dec<<" "<<get(g,id)<<"\n";
    }
      });
  }
  sblm_string const& get(G const& g,syntax_id_type sid) const {
    G::scored_syntax_type ss=g.get_scored_syntax(sid);
    assert(ss.syntax_id()==sid);
    assert(!ss.no_contig_id());
    return contig_sblmstrs.at_assert(ss.contig_id);
  }
};

template <class C, class T>
void print(std::basic_ostream<C,T>& o, sblm_info const& d, LM const& lm)
{
  word_spacer_c<','> sp;
  for (info_lmids::const_iterator w=d.cats.begin(),ew=d.cats.end();w!=ew;++w)
    o<<sp<<lm.word(*w);
}


// copied once into a holder (type eraser)
class sblm_info_factory {
public:
  // required by interface
  typedef sblm_info info_type;


  typedef tuple<info_type,score_t,score_t> result_type; // info, inside-score, heuristic
  static inline sblm_info & info(result_type &r) {
    return boost::get<0>(r);
  }
  static inline score_t & inside(result_type &r) {
    return boost::get<1>(r);
  }
  static inline score_t & h(result_type &r) {
    return boost::get<2>(r);
  }

  template<class Grammar>
  sblm_info_factory(Grammar& g,
                    const lattice_tree& lattice,
                    const property_map_type & pmap,
                    load_lm const& lm,
                    treebank_categories const& tbc,
                    bool bigrams,
                    bool pcbigrams,
                    bool greedy
    )
    : tbc(tbc)
    , sblm_fid(get_fid(g,sblm_featname))
    , sblm_unkword_fid(get_fid(g,sblm_unkword_featname))
    , sblm_unkcat_fid(get_fid(g,sblm_unkcat_featname))
    , sblm_pword_fid(get_fid(g,sblm_pword_featname))
    , sblm_nts_fid(get_fid(g,sblm_nts_featname))
    , sblm_wt(get_wt(g,sblm_featname))
    , sblm_unkword_wt(get_wt(g,sblm_unkword_featname))
    , sblm_unkcat_wt(get_wt(g,sblm_unkcat_featname))
    , sblm_pword_wt(tbc.pword ? get_wt(g,sblm_pword_featname) : 0)
    , sblm_nts_wt(get_wt(g,sblm_nts_featname))
    , dict(g.dict())
    , lm(lm)
    , tokensp(new token_to_sblm(tbc,dict,lm.lm()))
    , bigrams(bigrams)
    , pcbigrams(pcbigrams)
    , namersp(new namer_t(sblm_bifeatpre,lm_str2id_raw(lm.lm()),g.feature_names(),true))
    , pcnamersp(new namer_t(sblm_pcfeatpre,lm_str2id_raw(lm.lm()),g.feature_names(),true))
  {
    SBMT_INFO_STREAM(sblm_domain,"sblm factory weight="<<sblm_wt);
    sblm::greedy=greedy;
    debug::dbg.g=&g;
    debug::dbg.lm=&lm.lm();
    sblmstr.init(g,*tokensp,lm.lm());
    if (bigrams) namersp->set_weights(g.get_weights());
    if (pcbigrams) pcnamersp->set_weights(g.get_weights());
  }

  typedef indicator_namer<lm_str2id_raw,bigram_feat_name,graehl::spin_locking> namer_t;


  ////////////////////////////////////////////////////////////////////////////
  ///
  ///  result_generator: required by interface
  ///  info factories are allowed to return multiple results for a given
  ///  set of constituents.  it returns them as a generator.
  ///  a generator is a result_type functor(void) object that is convertible to
  ///  bool.  the generator converts to false when there are no more results
  ///  to retrieve.  this is analogous to an input iterator.
  ///
  ///  your generator will be called like so:
  ///  while (generator) { result_type res = generator(); }
  ///
  ///  for those that are familiar with python/ruby, yes there are libraries that
  ///  embue c++ with equivalent generator/coroutine behavior
  ///
  ///  if your create_info method only ever returns one result, you
  ///  can use single_value_generator as your result_generator
  ///
  ////////////////////////////////////////////////////////////////////////////
  typedef gusc::single_value_generator<result_type> result_generator;

  template <class G>
  sblm_string const& get_sblmstr(G &g,typename G::rule_type rule) {
    syntax_id_type sid=g.get_syntax_id(rule);
    assert(sid!=NULL_SYNTAX_ID); // foreign tokens don't have rules. everything else that's scoreable should have a syntax id
    return sblmstr.get(g,sid);
  }


  template <class ConstituentIterator,class G>
  result_generator
  create_info( G& grammar
               , typename G::rule_type rule
               , span_t const& span
               , iterator_range<ConstituentIterator> const& range
    )
  //code duplicated from component_scores below:
  {
    result_type ret;
    child_ps cps(range,false);
    score_t ind,s,word;
    weight_combine<namer_t> f(*namersp,ind),pcf(*pcnamersp,ind);
    unsigned unkwords,unkcats,nts;
    get_sblmstr(grammar,rule).compute_info(cps,info(ret).cats,s,h(ret),f,bigrams,pcf,pcbigrams,false,unkwords,unkcats,nts,word);
    SBLM_FINITE(s,"sblm");
    SBLM_FINITE(ind,"sblmind");
    s^=sblm_wt;
    s*=pow(word,sblm_pword_wt);
    s*=ind; // this part is already weighted by different feature weights than "sblm"
    s*=indicator_wt(unkwords*sblm_unkword_wt);
    s*=indicator_wt(unkcats*sblm_unkcat_wt);
    s*=indicator_wt(nts*sblm_nts_wt);
    SBLM_FINITE(s,"sblm-and-unks");
    inside(ret)=s;
    h(ret)^=sblm_wt;
    SBLM_FINITE(h(ret),"sblmh");
    return ret;
  }

  ////////////////////////////////////////////////////////////////////////////

  //TODO: support multi-lm for sblm ngram?
  template <class ConstituentIterator, class ScoreOutputIterator, class G>
  boost::tuple<ScoreOutputIterator,ScoreOutputIterator>
  component_scores( G& grammar
                    , typename G::rule_type rule
                    , span_t const& span
                    , iterator_range<ConstituentIterator> const& range
                    , info_type const& result
                    , ScoreOutputIterator out
                    , ScoreOutputIterator hout )
  {
    using namespace sbmt::io;
    scoped_domain_settings scpng(ngram_domain,lvl_verbose,sblm_domain,lvl_verbose);
    scoped_domain_settings scp(sblm_domain,1,sblm_domain,lvl_info);

    child_ps cps(range,false); // skip rhs terminals if any (lattice f->f rules)
    components_out<ScoreOutputIterator,namer_t> f(*namersp,out),pcf(*pcnamersp,out);
    info_lmids out_state_ignore;
    score_t inside,h,word;
    unsigned unkwords,unkcats,nts;
    get_sblmstr(grammar,rule).compute_info(cps,out_state_ignore,inside,h,f,bigrams,pcf,pcbigrams,true,unkwords,unkcats,nts,word);
    SBLM_FINITE(inside,"sblm(component)");
    SBLM_FINITE(h,"sblmh(component)");
    *out=component_score(sblm_fid,inside);++out; //postfix increment isn't working; why? any_output_iterator has value_type void?
    *out=component_score(sblm_unkword_fid,indicator_wt(unkwords));++out;
    *out=component_score(sblm_unkcat_fid,indicator_wt(unkcats));++out;
    *out=component_score(sblm_nts_fid,indicator_wt(nts));++out;
    if (tbc.pword)
      *out=component_score(sblm_pword_fid,word);++out;
    *hout=component_score(sblm_fid,h);++hout;
    return boost::make_tuple(out,hout);
  }

  template <class Grammar>
  bool scoreable_rule( Grammar const& grammar, typename Grammar::rule_type r ) const
  {
    ////!is_virtual_tag(grammar.rule_lhs(r));
    return grammar.get_syntax_id(r)!=NULL_SYNTAX_ID && !is_foreign(grammar.rule_lhs(r)); // exclude foreign -> foreign rules that put features on lattices
  }
  
  template <class Grammar>
  score_t
  rule_heuristic(Grammar& grammar, typename Grammar::rule_type r) const
  {
    syntax_id_type sid=grammar.get_syntax_id(r);
    return sid==NULL_SYNTAX_ID ? 1.0 : sblmstr.get(grammar,sid).total;
  }
  
  bool deterministic() const { return true; }

  template <class Grammar>
  string
  hash_string(Grammar const& grammar, info_type const& info) const
  {
    std::stringstream ssr;
    print(ssr,info);
    //std::cerr<<"Hash STRING: "<<ssr.str()<<std::endl;
    return ssr.str();
  }

  treebank_categories tbc;
  fid_t sblm_fid,sblm_unkword_fid,sblm_unkcat_fid,sblm_pword_fid,sblm_nts_fid;
  double sblm_wt,sblm_unkword_wt,sblm_unkcat_wt,sblm_pword_wt,sblm_nts_wt;
  indexed_token_factory& dict;
  load_lm lm;
  shared_ptr<token_to_sblm> tokensp;
  sblm_strings sblmstr;
  bool bigrams,pcbigrams;
  shared_ptr<namer_t> namersp,pcnamersp;
private:
};


} // namespace sblm


#endif
