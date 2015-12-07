# if ! defined(SBMT__EDGE__HEAD_HISTORY_DLM_INFO_HPP)
# define       SBMT__EDGE__HEAD_HISTORY_DLM_INFO_HPP

# include <vector>
# include <string>
# include <iostream>
# include <sbmt/edge/dlm_info.hpp>
# include <sbmt/dependency_lm/DLM.hpp>
# include <sbmt/edge/info_base.hpp>
# include <gusc/generator/single_value_generator.hpp>
# include <sbmt/edge/constituent.hpp>
# include <boost/function_output_iterator.hpp>
# include <sbmt/feature/accumulator.hpp>
namespace sbmt {

namespace {
struct nullout {
    template <class X> void operator()(X const& x) const { return; }
};
typedef boost::function_output_iterator<nullout> nulloutitr;
}

/// N: how many boundary words need to be memoized. The head
/// word is memo-ized regardless. We need to set N=0 when
/// using bilexical dlm, and set N=2 when using BBN style 
/// trigram dlm.
template<unsigned N, class LMID_TYPE = unsigned int>
class head_history_dlm_info 
  : public dlm_info<N, LMID_TYPE>
  , public info_base< head_history_dlm_info<N,LMID_TYPE> >
{
    typedef dlm_info<N, LMID_TYPE> __base;
public:

    LMID_TYPE head;
    typedef head_history_dlm_info  info_type;
 public:
    head_history_dlm_info& operator=(head_history_dlm_info const& o) 
     { __base::operator=(o); head = o.head; return *this; } 

    head_history_dlm_info() { head = (LMID_TYPE) -1; }
    
    //! compare the context words
    bool equal_to(info_type const& other) const
    {
        if(head == other.head) {
            return __base::equal_to(other);
        } else {
            return false;
        }
    }

    void set_head(const LMID_TYPE h) { head = h;}
    LMID_TYPE get_head() const { return head;}

    //! hash the context words.
    std::size_t hash_value() const; 
};

// greedy-search version of dlm.  notice equal_to and hash_value.
template<unsigned N, class LMID_TYPE = unsigned int>
class greedy_head_history_dlm_info 
  : public dlm_info<N, LMID_TYPE>
  , public info_base< greedy_head_history_dlm_info<N,LMID_TYPE> >
{
    typedef dlm_info<N, LMID_TYPE> __base;
protected:
    LMID_TYPE head; 
public:
    typedef greedy_head_history_dlm_info  info_type;
 public:
    info_type& operator=(info_type const& o) 
    { __base::operator=(o); head = o.head; return *this; } 

    greedy_head_history_dlm_info() { head = (LMID_TYPE) -1; }
    
    //! compare the context words
    bool equal_to(info_type const& other) const
    {
        return true;
    }

    void set_head(const LMID_TYPE h) { head = h; }
    LMID_TYPE get_head() const { return head; }

    //! hash the context words.
    std::size_t hash_value() const { return 0; }
};

////////////////////////////////////////////////////////////////////////////////

/*
// warning: not multi-LM safe.
template<class O,unsigned N,class LM>
O& operator <<(O& o, ngram_info<N,LM> const& info)
{
    info.print_self(o);
    return o;
}
*/

////////////////////////////////////////////////////////////////////////////////

template<class C, class T, class TF, class ID,unsigned N>
void print(std::basic_ostream<C,T> &o, head_history_dlm_info<N, ID> const& info, TF const&tf)
{
    o << "[";
    tf.print(o,&(info.head),(&(info.head)) + 1);
    o << "][";
    info.print(o,tf);
    o << "]";
}

template<class C, class T, class TF, class ID,unsigned N>
void print(std::basic_ostream<C,T> &o, greedy_head_history_dlm_info<N, ID> const& info, TF const&tf)
{
    return;
}

template<class C, class T, class TF, class ID,unsigned N, class LM>
void print(std::basic_ostream<C,T> &o, head_history_dlm_info<N, ID> const& info, LM const& lm)
{
    info.print(o,lm);
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned N, class LMID, bool Greedy>
struct select_head_history_dlm_info {
    typedef head_history_dlm_info<N, LMID> type;
};
     
template <unsigned N, class LMID>
struct select_head_history_dlm_info<N,LMID,true> {
    typedef greedy_head_history_dlm_info<N, LMID> type;
};

template <unsigned N, class LM, bool Greedy = false>
class head_history_dlm_info_factory : public dlm_info_factory<N, LM>
{
 public:
    typedef LM lm_type;        
    typedef typename lm_type::lm_id_type lm_id_type;

    typedef typename select_head_history_dlm_info<N,lm_id_type,Greedy>::type 
            info_type;
    typedef head_history_dlm_info_factory self_type;
        
    head_history_dlm_info_factory( boost::shared_ptr<MultiDLM> const& lm
                                       , property_map_type const& pmap
                                       , const indexed_token_factory& tf
                                       )
      : m_depLMs(lm)
      , lmstrid(pmap.find("dep_lm_string")->second) { init(const_cast<indexed_token_factory&>(tf)); }

    template <class Grammar, class Accumulator>
    score_t compute_dlm_delta_score(const Grammar&g, 
                                    const indexed_lm_string& lmstr, 
                                    Accumulator const& ind_dlm_scores) const;
    
    /// begin: new info interface //////////////////////////////////////////////
    template <class Grammar>
    score_t rule_heuristic( Grammar const& grammar
                          , typename Grammar::rule_type r ) const;
    
    bool deterministic() const { return true; }
    
    template <class Grammar>
    bool scoreable_rule( Grammar const& grammar
                       , typename Grammar::rule_type r ) const
    {
        return grammar.is_complete_rule(r);
        //if (grammar.rule_has_property(r,lmstrid)) assert(grammar.rule_lhs(r).type() != virtual_tag_token);
        //return grammar.rule_has_property(r,lmstrid);
    }
    
    typedef gusc::single_value_generator< boost::tuple<info_type,score_t,score_t> >
            result_generator;
    
    template <class Grammar, class ConstituentRange>
    result_generator
    create_info( Grammar const& grammar
               , typename Grammar::rule_type rule
               , span_t const& span
               , ConstituentRange const& constituents 
               )
    {
        boost::tuple<info_type,score_t,score_t> result;
        create_info( grammar
                   , rule
                   , constituents
                   , boost::get<0>(result)
                   , boost::get<1>(result)
                   , boost::get<2>(result)
                   , nulloutitr() );
        return result;
    }
    
    template < class Grammar
             , class ConstituentRange
             , class ScoreOutputIterator
             , class HeurOutputIterator >
    boost::tuple<ScoreOutputIterator,HeurOutputIterator>
    component_scores( Grammar const& grammar
                    , typename Grammar::rule_type rule
                    , span_t const& span
                    , ConstituentRange const& constituents
                    , info_type const& result
                    , ScoreOutputIterator scores_out
                    , HeurOutputIterator heur_out )
    {
        boost::tuple<info_type,score_t,score_t> res;
        create_info( grammar
                   , rule
                   , constituents
                   , boost::get<0>(res)
                   , boost::get<1>(res)
                   , boost::get<2>(res)
                   , scores_out
                   );
        return boost::make_tuple(scores_out,heur_out);
    }
    
    template <class Grammar>
    std::string hash_string(Grammar const& grammar, info_type const& info) const
    { 
        std::stringstream sstr;
        print(sstr,info,*m_depLMs);
        return sstr.str();
    }
    /// end: new info interface ////////////////////////////////////////////////

 protected:
    template <class Grammar, class Accumulator>
    score_t
    do_dlm_string(Grammar const& gram
                  , std::deque<std::string>& dlmEvents
                  //, std::vector<const edge<IT>*> edgevec
                  , lm_id_type* lB
                  , lm_id_type* rB
                  , lm_id_type& headid
                  , Accumulator const& ind_scores) const;
 private:

    void init(indexed_token_factory& tf);

    score_t heuristic_score(info_type const& info) const; 
                     // used only with identity lmstring, or for debugging
    
    /// convert the indexed (d)lm string into a deque of strings, with
    /// variable indexes replaced with the head word of the corresponding
    /// edge. 
    /// if there is any <PH> in front of the variable index, we replace the
    /// <PH> string with the (left/right) boundary word of the edge, 
    /// depending on whether it is an dlm in left direction (to head) or
    /// the right direction.
    /// -- The converted string vector will never be empty --- internally,
    ///    if it is empty, we put sequence "<H> <unk> <H>" in.
    template <class Gram>
    void deintegerize_dlm_string(const indexed_lm_string& lmstr, 
                                   Gram const& gram,
                                   std::vector<const info_type*> antecedent_ITs,
                                   std::deque<std::string>& vout) const ;

    // converts the event in the dlm string (containing vars) into a vector of words.
    // [start, end) is the range of the event in lmstr:  <E> <L> book a </E>, the the 
    // 'start' points to <book> and end points to </E>. end is not included in processing
    // in this function.
    // this is an auxilary function of deintegerize_dlm_string.
    template <class Gram>
    void process_an_event(std::deque<std::string>& eventsV, 
                     const indexed_lm_string& lmstr, 
                     int start, int end, 
                     bool event_dir, 
                     Gram const& gram,
                     std::vector<const info_type*> antecedent_ITs) const;

    template <class Grammar, class CR, class Accumulator>
    void
    create_info( Grammar const& grammar
               , typename Grammar::rule_type r
               , CR const& constituents
               , info_type& n
               , score_t& inside
               , score_t& heuristic
               , Accumulator const& scores_out
               );

    // I have to duplicate this because I dont want this method
    // to dependend on IT.
    template <class Gram>
    void compute_ngrams( info_type &n
                       , score_t &inside
                       , score_t &heuristic
                       , Gram& gram
                       , indexed_lm_string const& lmstr);
    
    
    
    //! The dependency LMs. Multiple ones in case the entire
    //! deplm is decomposed into different ones each for one
    //! type of event.
    //std::vector<boost::shared_ptr<lm_type> > m_depLMs;
    boost::shared_ptr<MultiDLM> m_depLMs;
    size_t lmstrid;

    indexed_token PH;
    indexed_token L;
    indexed_token R;
    indexed_token E;
    indexed_token E_close;
    indexed_token LB;
    indexed_token RB;
    indexed_token H;

};

} // namespace sbmt

#include "sbmt/edge/impl/head_history_dlm_info.ipp"


# endif //     SBMT__EDGE__HEAD_HISTORY_DLM_INFO_HPP
