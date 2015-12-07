#ifndef SBMT_EDGE_ngram_info_ipp_
#define SBMT_EDGE_ngram_info_ipp_

// FIXME: all std::vector impl. arrange their elements in a flat array
// and LW LM requires it, although I'm not sure std permits it?
// ANSWER: it is required by the standard that &v[0] return the location of
// contiguous memory containing the contents of the vector.
// however, there is no guarantee that v.begin() do the same
// (ie vector<X>::iterator need not be X*) --michael

#include <sbmt/grammar/rule_input.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <boost/scoped_array.hpp>
#include <algorithm>
#include <sbmt/edge/impl/lm_phrase.ipp>

/** ngram_info.hpp : the implementation of the ngram_info.
 */

/**
   right words: easy.  don't control flow, can easily shorten the suffix
   abc -> bc if abc is unknown
   left words: hard.  have to partially score abc -> p(c)*p_bo(ab) ...
   then later score p_bo(zab) when we prepend something ending in z.
   and if we prepend nothing, have to preserve in the left state the status
   of ab.

   just as we have 0, the null word, e.g. ab0 means that we have no gap,
   we have the backoff word *:
   ab* means when you place zab* you have to score p_bo(zab)
 */

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
// ngram_info methods
//
////////////////////////////////////////////////////////////////////////////////

// hash the boundary words.
// Do we have other good hash function for the boundary words?
template<unsigned N,class LM>
std::size_t ngram_info<N,LM>::hash_value() const {
  //FIXME: why is this better than determining length and then hashing? is the context_len version below somehow used?
    return hash_value(ctx_len);
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned N, class LM>
std::size_t ngram_info<N,LM>::hash_value(unsigned int context_len) const {
    const_iterator i = &lr[0][0];
    const_iterator e= i + context_len;
    std::size_t ret = 0;
    for(;i != e; ++i) boost::hash_combine(ret,*i);
    e = (&lr[1][0]) + ctx_len;
    i = e - context_len;
    for(; i != e; ++i) boost::hash_combine(ret,*i);
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned int N, class LM>
bool ngram_info<N,LM>::equal_to( info_type const& other
                               , unsigned short const& context_len ) const
{
    assert(context_len <= ctx_len);
    for (unsigned int i = 0; i < context_len; ++i) {
        if (lr[0][i] != other.lr[0][i]) return false;
        if (lr[1][ctx_len - i - 1] != other.lr[1][ctx_len - i - 1]) return false;
    }
    assert(this->hash_value(context_len) == other.hash_value(context_len));
    return true;
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned N,class LM>
template <class C, class T, class LMType>
std::basic_ostream<C,T>&
ngram_info<N,LM>::print( std::basic_ostream<C,T> &o
                       , LMType const& lm ) const
{
    const_iterator begin = &(lr[0][0]);
    const_iterator end = begin + ctx_len;
    lm.print(o,begin,end);
//    if (!no_gap()) {
        o << "//";
        begin = &(lr[1][0]);
        end = begin + ctx_len;
        lm.print(o,begin,end);
//    }
    return o;
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned N, class LM>
template <class C, class T>
std::basic_ostream<C,T>&
ngram_info<N,LM>::print_self(std::basic_ostream<C,T>& o) const
{
    // FIXME: expose LWVocab with printer and last_vocab in lm_type
    o<<"[";
    const_iterator begin = &(lr[0][0]);
    const_iterator end = begin + ctx_len;
    for (;begin != end; ++begin) o << *begin << ' ';
    //sbmt::print(o,begin,end);
//    if (!no_gap()) {
        o << "//";
        begin = &(lr[1][0]);
        end = begin + ctx_len;
        for (;begin != end; ++begin) o << *begin << ' ';
      //  sbmt::print(o,begin,end);
//    }
    o<<"]";
    return o;
}

////////////////////////////////////////////////////////////////////////////////
//
//  ngram_info_factory methods
//
////////////////////////////////////////////////////////////////////////////////
/*
template <unsigned N, class LM>
void ngram_info_factory<N,LM>::create_info(info_type& n)
{
    ngram_info_accessors<N,LM>::set_null(n);
}
*/
template <unsigned N,class LM> template <class Gram>
void ngram_info_factory<N,LM>::
compute_ngrams( info_type &n
              , score_t &inside
              , score_t &heuristic
              , Gram& gram
              , bool is_toplevel
              , typename ngram_rule_data<Gram>::return_type lmstr
              , lmkids const& lmk )
{
    //inside.set(as_one());
    //heuristic.set(as_one()); //FIXED: used to be called by edge builder with an already accumulated heuristic. then, it's just default initailized??? (via boost::tuple) - VERY sketchy, but fortunately, i guess, the default init of score_t (lognumber) is 1.0, so no real change
    assert(inside.is_one() && heuristic.is_one());
    compute_ngrams_generic( n
                          , impl::inside_accum(inside)
                          , impl::inside_accum(heuristic)
                          , gram
                          , is_toplevel
                          , lmstr
                          , lmk);
}

template <unsigned N,class LM> template <class Gram>
boost::tuple<component_scores_vec,component_scores_vec>
ngram_info_factory<N,LM>::
compute_ngrams( info_type &n
              , Gram& gram
              , bool is_toplevel
              , typename ngram_rule_data<Gram>::return_type lmstr
              , lmkids const& lmk )
{
    component_scores_vec s(lm.lm().n_components()); // named return val opt?
    component_scores_vec h(lm.lm().n_components());
    compute_ngrams_generic( n
                          , impl::component_accum(s)
                          , impl::component_accum(h)
                          , gram
                          , is_toplevel
                          , lmstr
                          , lmk );
    return boost::make_tuple(s,h);
}

template <unsigned N,class LM>
template <class Gram,class Accum,class AccumH>
void
ngram_info_factory<N,LM>::
compute_ngrams_generic( info_type &n
                      , Accum const& accum
                      , AccumH const& accum_heuristic
                      , Gram& gram
                      , bool is_toplevel
                      , typename ngram_rule_data<Gram>::return_type lmstr
                      , lmkids const& lmk )
{
    #if 0
    std::cerr << "\nINSIDE:\n";
    #endif
    LM &lm=this->lm.lm();

    if (lmstr.is_identity() && !is_toplevel) {
        n = *lmk[0];

# ifdef DEBUG_LM
        n.print(DEBUG_LM_OUT<<" - identity LMSTRING result=",lm)<<' ';
# endif
        SBMT_PEDANTIC_EXPR(ngram_domain,
        {
            continue_log(str) << "identity LMSTRING, result=";
            n.print(continue_log(str),lm);
        });
        #if 0
        std::cerr << "\nHEURISTIC\n";
        #endif
        heuristic_score(n,accum_heuristic);
        return;
    }

    impl::lm_phrase<N,LM> phrase(lmstr.size(),shorten_left,shorten_right);
    if (is_toplevel) // or (lmstr.begin()->is_token() and lm_id(lmstr.begin()->get_token()) == lm.start_id)) //TODO: or lmstring[0]==<s>
        phrase.start_sentence(lm);

#ifdef DEBUG_LM
    DEBUG_LM_OUT<<"\nLMSTRING:";
    for(typename ngram_rule_data<Gram>::type::const_iterator i = lmstr.begin(),end= lmstr.end();
        i != end; ++i)
        if(i->is_token())
            DEBUG_LM_OUT<<" T="<<print(i->get_token(),gram.dict());
        else if(i->is_index())
            DEBUG_LM_OUT<<" I="<<i->get_index();
    DEBUG_LM_OUT<<"\n ";
#endif
    for(typename ngram_rule_data<Gram>::type::const_iterator i = lmstr.begin(),end= lmstr.end();
        i != end; ++i) {
        // lexical item
        if(i->is_token()){
#ifdef DEBUG_LM
            DEBUG_LM_OUT << " T="
                         << print(i->get_token(),gram.dict())
                         << ' ';
#endif
            phrase.append_word(lm_id(i->get_token()),lm);
        }
        // variable
        else if(i->is_index()) {
            size_t varIndex = i->get_index();
#ifdef DEBUG_LM
            DEBUG_LM_OUT<<" I="<<varIndex<<' ';
#endif
            const info_type& kid = *lmk[varIndex];
            phrase.append_child(n,kid,lm,accum);
        } else {
            //FIXME: assert(0);// ?
        }

    }

    if (is_toplevel) phrase.end_sentence(lm);

    phrase.finish_last_phrase(n,lm,accum);
    #if 0
    std::cerr << "\nHEUR\n";
    #endif
    heuristic_score(n,accum_heuristic);
}

////////////////////////////////////////////////////////////////////////////////


template<unsigned N,class LM>
template <class Accum>
void
ngram_info_factory<N,LM>::heuristic_score(info_type const& it, Accum const& accum) const
{
    typedef ngram_info_accessors<N,LM> nia;
#ifdef DEBUG_LM
//    return score_t(as_one());
    DEBUG_LM_OUT << "\n(left words) heuristic for state: ";
    it.print(DEBUG_LM_OUT,lm.lm());
    DEBUG_LM_OUT<<": ";
#endif

    if (lm.lm().start_id == it(0,0)) return;
//    if (it.starts_sentence(*lm)) return score_t(as_one()); //not really necessary unless somebody made the mistake of letting TOP be pruned out by S, without <f> S -> TOP rule (diff. spans)
    LM &l=lm.lm();

    accum.sequence_prob(l,nia::left_begin(it),nia::left_begin(it),nia::left_end(it));
    //score_t ret=l.sequence_prob(nia::left_begin(it),nia::left_end(it));
    //SBMT_PEDANTIC_EXPR(ngram_domain, {
    //    continue_log(str) << "heuristic for state ";
    //    it.print(continue_log(str),l);
    //    continue_log(str)<<'(';
    //    it.print_self(continue_log(str));
    //    continue_log(str)<<')'<<"="<<ret;
    //});
    //return ret;
}


////////////////////////////////////////////////////////////////////////////////

template<unsigned N,class LM> template <class O>
void
ngram_info_factory<N,LM>::print_stats(O &o) const
{
#ifdef NGRAM_EXTRA_STATS
    impl::lm_phrase<N,LM>::print_stats(o);
#endif
    lm.lm().print_stats(o); // only if STAT_LM defined does this do anything
    o << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
template <class I>
void condinc(I& i, I&e) { if (i != e) ++i; }
 //TODO: cache not through hashtable, but as member of rule
template<unsigned N,class LM> template <class G>
score_t
ngram_info_factory<N,LM>::rule_heuristic( G& gram
                                        , typename G::rule_type r ) const
{
    score_t score=as_one();
    if (not gram.rule_has_property(r,lmstrid)) return score;
#ifdef DEBUG_LM
    DEBUG_LM_OUT << "\nRule ngram estimate for id="
                 << gram.get_syntax_id(r) << ": ";
#endif
    SBMT_PEDANTIC_EXPR(ngram_domain,{
        continue_log(str) << "rule ngram heuristic for syntax id="
                          << gram.get_syntax_id(r) << ":";
    });
    typename ngram_rule_data<G>::return_type 
        lms = ngram_rule_data<G>::value(gram,r,lmstrid);
    boost::scoped_array<lm_id_type> ngr(new lm_id_type[lms.size()]);
    lm_id_type *ngr_start=ngr.get();
    LM &l=lm.lm();
    for(typename ngram_rule_data<G>::type::iterator i=lms.begin(),e=lms.end(),j;
        i != e;condinc(i,e)) { // because we may increment one past end below
        if (i->is_token()) {
            lm_id_type *n=ngr_start;
            j=i;
            do {
                if (lm_id(j->get_token()) != l.start_id) {
                  *n++=lm_id(j->get_token());
                }
                condinc(j,e);
            } while(j!=e && j->is_token());
#ifdef DEBUG_LM
//            lm->print(DEBUG_LM_OUT,ngr_start,n);
            DEBUG_LM_OUT << " rule-phrase: ";
#endif
            score *=l.sequence_prob(ngr_start,n);
            i=j;
        }
    }
    return score;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt


#endif
