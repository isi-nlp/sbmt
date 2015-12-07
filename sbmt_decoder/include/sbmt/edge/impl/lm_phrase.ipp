#ifndef SBMT_EDGE_lm_phrase_ipp_
#define SBMT_EDGE_lm_phrase_ipp_

#include <sbmt/edge/ngram_info.hpp>
#include <boost/operators.hpp>
#include <iostream>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
//
// access methods that depend on the LM type. used to aid in the construction of
// ngram types, but putting them in ngram_info makes ngram_info dependent on
// LM details, which is a compilation headache, and defeats one of the benefits
//
////////////////////////////////////////////////////////////////////////////////
template <unsigned int N, class LM>
class ngram_info_accessors
{
public:
    typedef typename LM::lm_id_type lm_id_type;
    typedef LM lm_type;
    typedef ngram_info<N,lm_id_type> info_type;
    enum { ctx_len = info_type::ctx_len };
    typedef lm_id_type *iterator;
    typedef lm_id_type const* const_iterator;

    static const_iterator left_begin(info_type const& n)
    {
        return &(n(0,0));
    }

    static iterator left_begin(info_type& n)
    {
        return &(n(0,0));
    }

    static const_iterator left_end_full(info_type const& n) // may include NULLs
    {
        return &(n(0,ctx_len));
    }

    static iterator left_end_full(info_type& n) // may include NULLs
    {
        return &(n(0,ctx_len));
    }

    static const_iterator left_end(info_type const& n) // doesn't include NULL if any
    {
        const_iterator i=left_begin(n);
        const_iterator e=left_end_full(n);
        for (;i!=e;++i) {
            if (*i==(lm_id_type)lm_type::null_id
#ifdef LM_EQUIVALENT
                || *i==(lm_id_type)lm_type::bo_id
#endif
                )
                return i;
        }
        return e;
    }

    static const_iterator right_begin_full(info_type const& n)
    {
        return &(n(1,0));
    }

    static iterator right_begin_full(info_type& n)
    {
        return &(n(1,0));
    }

    static const_iterator right_begin(info_type const& n)
    {
        const_iterator i=right_end(n);
        const_iterator b=right_begin_full(n);
        for (;;) {
            --i;
            if (*i==(lm_id_type)lm_type::null_id)
                return i+1;
            if (i==b)
                return b;
        }
    }

    static const_iterator right_end(info_type const& n)
    {
        return &(n(1,ctx_len));
    }

    static iterator right_end(info_type& n)
    {
        return &(n(1,ctx_len));
    }

    static const_iterator gapless_begin(info_type const& n)
    {
        return left_begin(n);
    }

 private:
/// returns I such that [I,e) are all == k (*I==k or I==e).  allows negative range (e<b), returning just b.
    template <class I>
    inline static I find_end_not(I b,I e,lm_id_type k)
    {
        while(e>b) {
            --e;
            if (*e!=k)
                return e+1;
        }
        return b;
    }
 public:
    /// only call after you determine no_gap() (i.e. if it's full, this gives wrong answer)
    static const_iterator gapless_end(info_type const& n)
    {
        return find_end_not(left_begin(n),left_end_full(n)-1,lm_type::null_id);
        /*
          const_iterator b=left_begin();
          const_iterator e=left_end_full();
          while(b!=e) {
          --e;
          if (*e!=lm_type::null_id)
          return e+1;
          }
          return e;*/
    }

    // full context -> gap.  less than full -> know all words -> no gap.
    static bool no_gap(info_type const& n)
    {
        return n(0,ctx_len-1)==(lm_id_type)lm_type::null_id;
    }

    static bool needs_left_backoff(info_type const& n)
    {
        return n(0,ctx_len-1)==(lm_id_type)lm_type::bo_id;
    }

    /// only call after needs_left_backoff()
    static const_iterator left_backoff_end(info_type const& n)
    {
        return find_end_not(left_begin(n),left_end_full(n)-1,lm_type::bo_id);
    }

    static void set_start_sentence(info_type& n, lm_type const& lm)
    {
        for (unsigned m = 0; m < ctx_len; ++m)
            n(0,m) = lm.start_id;
    }

    static void set_end_sentence(info_type& n, lm_type const& lm)
    {
        for (unsigned m = 0; m < ctx_len; ++m)
            n(1,m) = lm.end_id;
    }

    static void set_null(info_type& n)// we can use this for foreign word edges, to be nice
    {
        for (iterator i=left_begin(n),e=right_end(n);i!=e;++i)
            *i=(lm_id_type)lm_type::null_id;
    }

    static bool is_null(info_type const& n)
    {
        return *left_begin(n)==(lm_id_type)lm_type::null_id;
    }
};

template <class LM>
class ngram_info_accessors<1,LM>
{
public:
    typedef LM lm_type;
    typedef typename LM::lm_id_type lm_id_type;
    typedef ngram_info<1,lm_id_type> info_type;
    enum { ctx_len = info_type::ctx_len };
    typedef lm_id_type *iterator;
    typedef lm_id_type const* const_iterator;

    static iterator left_begin(info_type const& n)
    { return 0; }

    static iterator left_end_full(info_type const& n)// may include NULLs
    { return 0; }

    static iterator left_end(info_type const& n) // doesn't include NULL if any
    { return 0; }
    static iterator left_backoff_end(info_type const& n)
    { return 0; }
    static bool needs_left_backoff(info_type const& n)
    { return false; }


    static iterator right_begin_full(info_type const& n)
    { return 0; }

    static iterator right_begin(info_type const& n)
    { return 0; }
    static iterator right_end(info_type const& n)
    { return 0; }
    static iterator gapless_begin(info_type const& n)
    { return 0; }
    static iterator gapless_end(info_type const& n)
    { return 0; }

    static bool no_gap(info_type const& n)
    { return false; }

    static void set_null(info_type& n) {}

    static void set_top(info_type& n) {}

    static void set_start_sentence(info_type& n, lm_type const& lm)
    { }

    static void set_end_sentence(info_type& n, lm_type const& lm)
    { }
};

#ifdef NGRAM_EXTRA_STATS
struct ngram_border_stats : boost::additive<ngram_border_stats>
{
    typedef unsigned long size_t;
    std::size_t n_border_words,n_border_words_reduced,n_items;
    ngram_border_stats()
    {
        reset();
    }
    void reset()
    {
        n_border_words_reduced=n_border_words=n_items=0;
    }
    void add(unsigned raw,unsigned reduced)
    {
        n_border_words+=raw;
        n_border_words_reduced+=reduced;
    }
    void record_left(unsigned raw,unsigned reduced)
    {
        ++n_items;
        add(raw,reduced);
    }
    void record_right(unsigned raw,unsigned reduced)
    {
        add(raw,reduced);
    }

    ngram_border_stats& operator += (ngram_border_stats const& o)
    {
        n_border_words_reduced+=o.n_border_words_reduced;
        n_border_words+=o.n_border_words;
        n_items+=o.n_items;
        return *this;
    }
    ngram_border_stats& operator -= (ngram_border_stats const& o)
    {
        n_border_words_reduced-=o.n_border_words_reduced;
        n_border_words-=o.n_border_words;
        n_items-=o.n_items;
        return *this;
    }
    template <class O>
    void print(O &o) const
    {
        float navg=n_border_words/(float)n_items;
        float ravg=n_border_words_reduced/(float)n_items;
        o << "LM item border words: "<<n_border_words<<" ("<<navg<<" average) reduced (by LM equivalence) to "<<n_border_words_reduced<<" ("<<ravg<<" average) over " << n_items << " items";
    }

};
#endif

namespace impl {

struct inside_accum
{
    score_t *pi;
    inside_accum(score_t &inside) :pi(&inside) {}
    inside_accum(inside_accum const& o) :pi(o.pi){}
    template <class LM,class lm_id_type>
    inline void bow_interval(LM const&lm,lm_id_type const*p,lm_id_type const*sep,lm_id_type const*end) const
    {
        #if 0
        std::cerr << "\nBOW:";
         
        if (sep < end) {
            BOOST_FOREACH(lm_id_type lid,std::make_pair(sep,end)) { std::cerr << ' ' << lm.word(lid); }
            std::cerr << " | ";
            BOOST_FOREACH(lm_id_type lid,std::make_pair(p,sep)) { std::cerr << ' ' << lm.word(lid); }
        } else {
            BOOST_FOREACH(lm_id_type lid,std::make_pair(p,end)) { std::cerr << ' ' << lm.word(lid); }
        } 
        #endif
        score_t pp = lm.bow_interval(p,sep,end);
        #if 0
        std::cerr << " ==> " << pp << "\n";
        #endif
        *pi *= pp;
    }
    template <class LM,class lm_id_type>
    inline void sequence_prob(LM const&lm,lm_id_type const*p,lm_id_type const*sep,lm_id_type const*end) const
    {
        #if 0
        std::cerr << "\nSEQ:";
        if (sep < end) {
            BOOST_FOREACH(lm_id_type lid,std::make_pair(sep,end)) { std::cerr << ' ' << lm.word(lid); }
            std::cerr << " | ";
            BOOST_FOREACH(lm_id_type lid,std::make_pair(p,sep)) { std::cerr << ' ' << lm.word(lid); }
        } else {
            BOOST_FOREACH(lm_id_type lid,std::make_pair(p,end)) { std::cerr << ' ' << lm.word(lid); }
        } 
        #endif 
        score_t pp = lm.sequence_prob(p,sep,end);
        #if 0
        std::cerr << " ==> " << pp << "\n";
        #endif
        *pi *= pp;
    }
};

struct component_accum
{
    component_scores_vec *ps;
    component_accum(component_scores_vec &component) :ps(&component) {}
    component_accum(component_accum const& o) :ps(o.ps){}
    template <class LM,class lm_id_type>
    inline void bow_interval(LM const&lm,lm_id_type const*p,lm_id_type const*sep,lm_id_type const*end) const
    {
        lm.bow_interval(*ps,p,sep,end);
    }
    template <class LM,class lm_id_type>
    inline void sequence_prob(LM const&lm,lm_id_type const*p,lm_id_type const*sep,lm_id_type const*end) const
    {
        lm.sequence_prob(*ps,p,sep,end);
    }
};


template <unsigned N,class LM>
struct lm_phrase
{
    /* accumulate (component) insides from LM:

       void bow_interval(lm,phrase,phrase_sep,phrase_end)

       void sequence_prob(lm,phrase,ctx_end,phrase_end)
     */


    typedef LM lm_type;
    typedef ngram_info_accessors<N,LM> nia_t;
#ifdef NGRAM_EXTRA_STATS
    template <class O> static void print_stats(O& o)
    {
        border_stats.print(o);
    }
#endif

    typedef typename lm_type::lm_id_type lm_id_type;
    typedef ngram_info<N,lm_id_type> info_type;
    typedef lm_id_type *iterator;
    typedef lm_id_type const* const_iterator;

    bool shorten_left,shorten_right;

    lm_phrase(unsigned lmstr_len,bool shorten_left=true,bool shorten_right=true) : shorten_left(shorten_left),shorten_right(shorten_right)
    {
        phrase=new lm_id_type[lmstr_len*2*N];
        reset();
    }

    ~lm_phrase()
    {
        delete[] phrase;
    }

    void append_word(lm_id_type i, lm_type const& lm)
    {
        //if (lm.start_id == i) assert(phrase == phrase_end);
        *phrase_end++=i;
        if (lm.start_id == i) ctx_end = phrase_end;
    }

    void start_sentence(lm_type const& lm)
    {
        append_word(lm.start_id,lm);
    }

    void end_sentence(lm_type const& lm)
    {
        append_word(lm.end_id,lm);
    }

    template <class A>
    void append_child(info_type &result,info_type const& kid,lm_type const& lm,A &accum)
    {
#ifdef DEBUG_LM
        kid.print(DEBUG_LM_OUT<<" child=",lm)<<" ";
#endif
        if (nia_t::no_gap(kid)) {
            append_words(nia_t::gapless_begin(kid),nia_t::gapless_end(kid),lm);
            return;
        }
        // kid.left: may end in BO
        const_iterator left_begin=nia_t::left_begin(kid);
        if (nia_t::needs_left_backoff(kid)) {
            const_iterator left_bo_end=nia_t::left_backoff_end(kid);
            const_iterator phrase_sep=phrase_end;
            append_words(left_begin,left_bo_end,lm);
            // finish scoring backoffs for removed words:
            accum.bow_interval(lm,phrase,phrase_sep,phrase_end);
/*
  [abcd] [123#] need p_bo(d123)
  .  also [abcd][#] ... need to pay bo for d, cd, bcd, abcd.
  intermediate cases: [abcd][1#] ... bo: d1, cd1, bcd1.
*/
            // assert: must be first (finish_left) phrase if length from phrase->phrase_end < ctxlen
//            assert(!left_done || phrase_end>=ctx_end);
            // same as use_phrase but we add the backoff word to the state
            for (;ctx_end < phrase_end; ++ctx_end)
                accum.sequence_prob(lm,std::max(phrase,ctx_end-ctxlen),ctx_end,ctx_end + 1);
            finish_left(result,lm,accum,lm_type::bo_id);
        } else {
            append_words(left_begin,nia_t::left_end(kid),lm);
            use_phrase(result,lm,accum);
        }
        clear();
        append_words(nia_t::right_begin(kid),nia_t::right_end(kid),lm);
        ctx_end=phrase_end;
    }

    void clear()
    {
        phrase_end=phrase;
    }


    template<class A>
    void finish_last_phrase(info_type &result,lm_type const &lm,A &accum)
    {
        use_phrase(result,lm,accum);
        finish_right(result,lm);
    }

 private:
    BOOST_STATIC_CONSTANT(unsigned,ctxlen=N-1);

    // private: don't reuse this phrase object on different items as the memory preallocated should increase w/ length of lm string
    void reset()
    {
        phrase_end=phrase;
        ctx_end=phrase+ctxlen;
        left_done=false;
    }

        template <class A>
    void use_phrase(info_type &result,lm_type const &lm,A &accum)
    {
        for (;ctx_end < phrase_end; ++ctx_end)
            accum.sequence_prob(lm,std::max(phrase,ctx_end-ctxlen),ctx_end,ctx_end + 1);
        finish_left(result,lm,accum);
    }


    void append_words(const_iterator b,const_iterator e,lm_type const& lm)
    {
        bool start = ((b != e) and (*b == lm.start_id));
        while (b!=e) {
            append_word(*b++,lm);
            if (start) ctx_end = phrase_end < (phrase + ctxlen) ? phrase_end : (phrase + ctxlen);
        }
    }

    unsigned size() const
    {
        return phrase_end-phrase;
    }

    bool is_start_sentence_phrase(lm_type const& lm) const
    {
        return phrase!=phrase_end && phrase[0]==lm.start_id;
    }

    bool is_end_sentence_phrase(lm_type const& lm) const
    {
        return phrase!=phrase_end && phrase_end[-1]==lm.end_id;
    }


    /// builds result left state.
    template <class Accum>
    void finish_left(info_type &result,lm_type const& lm,Accum &accum,lm_id_type terminator=lm_type::null_id)
    {
        if (left_done)
            return;
        left_done=true;
        //TODO: since edge forces TOP items into same equiv already, preserve actual context so foreign start/end get predicted based on full context?  nah, why bother p foreign start/end given e start/end = 1.
        if (is_start_sentence_phrase(lm)) { // no heuristic or further backoffs
            nia_t::set_start_sentence(result,lm);
        } else {
            typedef typename info_type::iterator lwit;
            lwit lw = nia_t::left_begin(result),
                 le = nia_t::left_end_full(result);
            const_iterator phrase_ctx_end=phrase+ctxlen;
            if (phrase_ctx_end > phrase_end)
                phrase_ctx_end = phrase_end;
            const_iterator orig_ctx_end=phrase_ctx_end;
            if (shorten_left) {
                phrase_ctx_end=lm.longest_prefix(phrase,phrase_ctx_end);
                if (phrase_ctx_end!=orig_ctx_end) { // FIXME: (prove ok) - what if: terminator = bo already (short phrase), and we still shorten result ???
// [phrase_ctx_end,phrase_end) to be inside scored
    SBMT_PEDANTIC_EXPR(ngram_domain,
                       {
                           continue_log(str) << "left words shortened to !";lm.print(continue_log(str),phrase,phrase_ctx_end,orig_ctx_end);
                       });

    accum.sequence_prob(lm,phrase,phrase_ctx_end,orig_ctx_end);
// since [phrase,phrase_ctx_end+1) is an unseen phrase, we can completely score the final words
// note: this results in p_bo([phrase,phrase_ctx_end)) being applied, but that may not be the full ctxlen, so p_bo([phrase-1,s)), p_bo([phrase-2,s)) ... will need to be applied later in append_child
                    /* // funny thing: lm.find_bow(phrase,phrase_ctx_end) was already included above.  to see why, assume a,b!!c.  a,b was in the lm, but a,b,c was not.  so, already pay p_bo(a,b) and compute p(b!!c).
                     */
                    terminator=lm_type::bo_id;
                }
            }
#ifdef NGRAM_EXTRA_STATS
            border_stats.record_left(orig_ctx_end-phrase, phrase_ctx_end-phrase);
#endif
#ifdef DEBUG_LM
            DEBUG_LM_OUT << " heuristic ";
#endif
            lwit i=std::copy((const_iterator)phrase,phrase_ctx_end,lw);
            while(i<le) *i++=terminator;
        }
    }

    /// build result right state
    void finish_right(info_type &result,lm_type const& lm)
    {
        if (is_end_sentence_phrase(lm)) {
            nia_t::set_end_sentence(result,lm);
            return;
        }
        typename info_type::iterator rb = nia_t::right_begin_full(result),
                                     r,
                                     re = nia_t::right_end(result);
        const_iterator pstart=phrase_end-ctxlen;
        if (pstart<phrase)
            pstart=phrase;
        const_iterator pstart_shorter= shorten_right ?
            lm.longest_suffix(pstart,phrase_end) :
            pstart;

#ifdef NGRAM_EXTRA_STATS
        border_stats.record_right(phrase_end-pstart,phrase_end-pstart_shorter);
#endif
        unsigned rlen=phrase_end-pstart_shorter;
        r=re-rlen;
        std::copy(pstart_shorter,(const_iterator)phrase_end,r);
        while (r>rb)
            *--r=lm_type::null_id;
#ifdef DEBUG_LM
        result.print(DEBUG_LM_OUT<<" result=",lm)<<" ";
#endif
    }

    static ngram_border_stats border_stats; //FIXME: thread local/combine or atomic ops
    lm_id_type *phrase,*phrase_end,*ctx_end;
    bool left_done;

};

#ifdef NGRAM_EXTRA_STATS
template <unsigned N,class LM>
ngram_border_stats lm_phrase<N,LM>::border_stats;
#endif

}

}
#endif
