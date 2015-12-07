/** LWNgramLM.h -- the LW ngram language model.
 * $(Id)$.
 */

/* actual used interface (by ngram_info):

longest_suffix,longest_prefix,sequence_prob,bow_interval

(sequence_prob implemented on top of prob)

that's it.

(and implictly: grammar's lmstring vocab mappings)

encapsulate open/closed/numclass LM options?  can vary over multiple LM combination?  why not.

text in -> filename/params factory? (parse combined lm from string?)  describe sequence/combinator separate from open/closed, lmfile, atnumclass options?

*/

#ifndef SBMT_NGRAM__LWNgramLM_hpp_
#define SBMT_NGRAM__LWNgramLM_hpp_

#include <string>
#include <deque>

#include <boost/bind.hpp>

#include <graehl/shared/word_spacer.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/hash/oa_hashtable.hpp>
#include <sbmt/hash/functors.hpp>
//TODO: install LW header/libs?  this is a thin wrapper and I'd prefer the wrapping to be inlined, not linked
#include <sbmt/ngram/base_ngram_lm.hpp>

namespace LW {
class LangModel;
class LWVocab;
class LangModelImplBase;
}

namespace sbmt {

/**
   \defgroup LM Ngram LM

We use the Language Weaver ngram LM (specifically the sorted array variant that saves a bit of memory, although it's not known how much it saves over the newer SRILM that also uses sorted arrays).

In order to increase LM recombination in our search, I needed some new functionality from the LM, which I've already added.

I've commited to the "sbmt" subversion at ISI some new LM API, that is implemented only for the KN and SA LMs.

In LangModelImplBase.h:

    /// looks for exactly the string [b,end) and returns BOW_NONE if no LM entry
    /// exists.  otherwise, returns the backoff prob of that history.  note: if
    /// end-b = maxorder, then the pointer returned will be the prob. (suggested
    /// change: return true/false if found, and backoff and/or prob separately)
    virtual logprob find_bow(iterator b,iterator end) const = 0;

    /// returns r such that [b,r) is the longest string which has an LM probability
    virtual iterator longest_prefix(iterator b,iterator end) const
    {
        for (;end>b;--end)
            if (find_bow(b,end)!=BOW_NONE)
                break;
        return end;
    }

    /// returns r such that [r,end) is the longest string which has an LM probability
    virtual iterator longest_suffix(iterator b,iterator end) const


(the longest_* methods are virtual because particular LMs may be able to more efficiently find prefixes or suffixes, e.g. I specialized longest_prefix in LangModelSA to exploit the left->right trie.)

Other changes:
  getContextProb in LangModelSA now performs half the lookups when accumulating backoffs (when a highest-order ngram entry isn't immediately found)

LangModel::ProbLog LangModelSA::getContextProb(iterator pContext, unsigned int nOrder)
{
    iterator end=pContext+nOrder;
    if (nOrder > max_order()) {
        nOrder = max_order();
        pContext=end-nOrder;
    }
    log_prob p=0;
    while (!prob_starting(pContext++,end,p)) ;
    return p;
}


Some useless virtualization removed (specifically: LangModelImpl).

Now, some comments about the LW LangModelSA class:

While the choice of data structure for a trie is pretty clever/compact (leaving
out only bit-shaving techniques that sacrifice # of words or precision in
logprobs), the order of the ngrams is all wrong (for the common p(word|history)
- the only method really used so far)

Suppose you have phrase:

w0 w1 w2 w3

and you're going to compute p(w3|w0...w2)

What's done now (wrong): is to store in the trie (left->right) "w0 w1 w2 w3", and if that's not found, look up "w0 w1 w2" for the backoff cost, and "w1 w2 w3" for the backed off prob (recursing).

I have improved this somewhat so that instead of 2*n trie lookups, only n are used (if we get to "w0 w1 w2", we save the backoff then, just in case we fail to see "w3" next.

However, the ideal trie organization would be:

right->left context, followed by (max order) word.  e.g.:

w2,w1,w0.w3 (.w3 means we're scoring p(w3|preceding context)

Here's what we'd do:

.w3 (unigram prob)
BO:w2 (accumulate backoff)
w2.w3 (bigram prob - if found, clear backoff)
BO:w2,w1 (accum backoff)
w2,w1.w3 (trigram - if found, clear backoff)
BO:w2,w1,w0 (accum backoff)
w2,w1,w0.w3 (max order)

Note: early exit if extending the history with ,wn gives a failed lookup.  use the most recent found prob and all the accumulated backoff

Advantage: trie node lookups are as few as possible.

Disadvantage: look up two words per level.  If every max-n-gram is known, then simple right->left (or left->right, doesn't matter) would do only N words lookup, whereas 2N-1 w/ this approach.

Alternative: strict right->left

w3,w2,w1,w0

w3 (unigram)
w3,w2 (bigram)
w3,w2,w1 (3gram - now, suppose this isn't found)
BO:w2,w1 (start backing off at w1 - the word we just failed to find above)
BO:w2,w1,w0

The right->leftness is best when we're interested in scoring a particular word.

Note that in all cases the data structures can be the same.  In the w2,w1,w0.w3 variant, the prob attached to a node in the trie "a b" is p(b|a), while the bow is for backing off from "b a" (confusing? not really).  In the strict left->right and right->left, the bow and prob are for the same sequence of words.

Finally: if we're building on 64bit, we should do away with the segmented array (better performance).  Is there a define that's always available on 64 bit?  in templates, we can test sizeof(long).  or we can create our own define so 32-bit users who know their OS allocator isn't lame can use flat arrays too.

*/



//! LWNgramLM: the LW ngram language model.

class LWNgramLM_impl : public base_ngram_lm<unsigned>
{
    typedef base_ngram_lm<unsigned> Base;
 public:
    typedef LW::LWVocab Vocab;
    Vocab *vocab;
    static Vocab const*last_vocab;
    static unsigned max_supported_order();

    typedef Base::lm_id_type lm_id_type;
    typedef Base::iterator iterator;
    typedef Base::const_iterator const_iterator;

    LWNgramLM_impl() : vocab(NULL),m_langModel(0)
    {}

    ~LWNgramLM_impl();

    score_t
    open_prob_len_raw(lm_id_type const* ctx,unsigned len) const;

    void set_weights_raw(weight_vector const& weights, feature_dictionary const& dict) {}

    std::string const& word_raw(lm_id_type id) const;

    static std::string const& word_recent(lm_id_type id);

    /** convert string to lw lm id.  doesn't use replace_digit*/
    lm_id_type id_raw(std::string const& tok,bool add_unk=false);

 public:

    /// returns r such that [b,r) is the longest known history (i.e. length maxorder-1 max)
    const_iterator longest_prefix_raw(const_iterator i,const_iterator end) const;

#if 0
        // note: use inlined fat_ngram_lm version instead since LW suffix is not more efficient

    /// reverse of longest_prefix (returns r such that [r,end) is the longest seen history)
        const_iterator longest_suffix_raw(const_iterator i,const_iterator end) const;
#endif

    score_t find_bow_raw(const_iterator i,const_iterator end) const;

    score_t find_prob_raw(const_iterator i,const_iterator end) const;


    //! throws runtime_error on failure.  LW (binary) LM only
    void read(std::string const& filename);

    void clear();

    unsigned int max_order_raw() const ;

    bool loaded() const
    {
        return m_langModel != NULL;
    }

 protected:
    //! The LW LM object.
    LW::LangModel* m_langModel;
    LW::LangModelImplBase* m_imp;

};

class LWNgramLM : public fat_ngram_lm<LWNgramLM,unsigned>,public LWNgramLM_impl
{
    typedef fat_ngram_lm<LWNgramLM,unsigned> Fat;
    friend struct fat_ngram_lm<LWNgramLM,unsigned>;
    typedef LWNgramLM_impl Impl;
 public:
    typedef Impl::lm_id_type lm_id_type;
    typedef Impl::iterator iterator;
    typedef Impl::const_iterator const_iterator;

    LWNgramLM(ngram_options const& opt=ngram_options())
        : Fat("lw",opt)
    {
    }

    void clear()
    {
        Fat::clear();
        Impl::clear();
    }

    void read(std::string const& filename)
    {
        opt.check();
        loaded_filename=filename;
        Impl::read(filename);
        Fat::log_describe();
        Fat::finish();
    }

    /// all these are boring/annoying ambiguity resolutions so Fat can work:

    const_iterator longest_prefix_raw(const_iterator i,const_iterator end) const
    { return Impl::longest_prefix_raw(i,end); }

#if 0
    /// reverse of longest_prefix (returns r such that [r,end) is the longest seen history)
    const_iterator longest_suffix_raw(const_iterator i,const_iterator end) const
    { return Impl::longest_suffix_raw(i,end); }
#endif

    // returns score_t(as_zero()) if the string doesn't exist, prob if end-i = maxorder, bo otherwise
    score_t find_bow_raw(const_iterator i,const_iterator end) const
    { return Impl::find_bow_raw(i,end); }

    score_t
    open_prob_len_raw(lm_id_type const* ctx,unsigned len) const
    { return Impl::open_prob_len_raw(ctx,len); }

    std::string const& word_raw(lm_id_type id) const
    { return Impl::word_raw(id); }

    lm_id_type id_raw(const std::string& tok,bool add_unk=false)
    { return Impl::id_raw(tok,add_unk); }

    bool loaded() const
    { return Impl::loaded(); }

    unsigned max_order_raw() const
    { return Impl::max_order_raw(); }

//    void print_further_sequence_details(std::ostream &o,const_iterator history_start,const_iterator score_start,const_iterator end) const
//    { Impl::print_further_sequence_details(o,history_start,score_start,end); }

};



// not multi-LM safe.
template <class O>
void print(O& o, LWNgramLM_impl::lm_id_type const* i,LWNgramLM_impl::lm_id_type const* end)
{
    graehl::word_spacer sep(',');
    for(; i < end; ++i)
        o << sep << LWNgramLM_impl::word_recent(*i);
}

} // end namespace sbmt

#endif
