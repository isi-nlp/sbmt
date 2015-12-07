# if ! defined(SBMT__SEARCH__FORCE_SENTENCE_FILTER_HPP)
# define       SBMT__SEARCH__FORCE_SENTENCE_FILTER_HPP

# include <sbmt/grammar/features_byid.hpp>
# include <sbmt/grammar/text_features_byid.hpp>
# include <sbmt/grammar/score_combiner.hpp>
# include <sbmt/search/span_filter_interface.hpp>
# include <sbmt/search/intersect_edge_filter.hpp>
# include <sbmt/hash/substring_hash.hpp>
# include <sbmt/token/indexed_token.hpp>
# include <sbmt/sentence.hpp>
# include <sbmt/edge/sentence_info.hpp>
# include <sbmt/chart/chart.hpp>

# include <vector>
# include <iterator>

# include <boost/iterator/iterator_facade.hpp>

namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////
///
///  based upon a given target sentence, represented by match,
///  returns span_strings that represent locations where lmstr could match.
///  \pre  span_str_out must be an insertion iterator over a container of
///        span_string objects.
///
////////////////////////////////////////////////////////////////////////////////
template <class InsertItrT, class LMSTRING> void
span_strings_from_lm_string( LMSTRING const& lmstr
                           , substring_hash_match<indexed_token> const& match
                           , InsertItrT span_str_out );
                           
////////////////////////////////////////////////////////////////////////////////
///
///  the force-sentence-filter uses this partial grammar interface to expand
///  the interface of grammar to include a span-sentence describing what part
///  of the english sentence constraint a rule is intended to match.
///  force-grammar holds onto GT by reference, it is not encapsulating it.
///  force-grammar should be fed to filter-bank for force-decoding.
///
////////////////////////////////////////////////////////////////////////////////
template <class GT>       
struct force_grammar : boost::noncopyable
{
    typedef indexed_binary_rule binary_rule_type;
    
    //typedef typename GT::scored_syntax scored_syntax;
    
    typedef typename GT::syntax_rule_type syntax_rule_type;
    typedef typename GT::scored_syntax_type scored_syntax_type;
    
    typedef std::pair<typename GT::rule_type, span_string> pair_t; // note: for now it's important to have the pair in this order, so pair_t * can be converted to GT::rule_type *, for edge.hpp to not need grammar arg to get syntax id
    typedef boost::shared_ptr<pair_t> pair_ptr_t;
    
    typedef stlext::hash_map< typename GT::rule_type
                            , std::list<pair_ptr_t>
                            , boost::hash<typename GT::rule_type> >
            rule_match_map_t;
            
    rule_match_map_t rule_matches;
    GT& gram;
    size_t lm_scoreable_id;
    size_t lm_string_id;
    
    typedef pair_t const*               rule_type;
    typedef typename GT::token_type     token_type;
    
    class rule_iterator : public boost::iterator_facade<
                                     rule_iterator
                                   , rule_type const
                                   , boost::forward_traversal_tag 
                                 >
    {
        typename GT::rule_iterator ritr;
        typename GT::rule_iterator rend;
        typename std::list<pair_ptr_t>::const_iterator sitr;
        typename std::list<pair_ptr_t>::const_iterator send;
        rule_match_map_t const* matches;
        rule_type current;
        
        void advance_ritr();
        void increment();
        bool equal(rule_iterator const& other) const;
        rule_type const& dereference() const { return current; } 
        
        rule_iterator( typename GT::rule_iterator
                     , typename GT::rule_iterator
                     , rule_match_map_t const& );
                     
        friend class force_grammar;
        friend class boost::iterator_core_access;
    public:
        rule_iterator();
    };
    
    typedef dictionary<in_memory_token_storage> dict_t;
    dict_t & dict() { return gram.dict(); }
    dict_t const& dict() const { return gram.dict(); }
    
    std::string const& label(token_type x) const { return gram.label(x); }
    
    typedef itr_pair_range<rule_iterator> rule_range;

    typename GT::token_factory_type tf() const 
    { return gram.dict(); }
    
    rule_range all_rules() const;
    rule_range unary_rules(token_type rhs) const;
    rule_range binary_rules(token_type first_rhs) const;
    
    rule_range toplevel_unary_rules(token_type rhs) const;
    rule_range toplevel_binary_rules(token_type first_rhs) const;
    scores_type rule_scores(rule_type r) const
    { return gram.rule_scores(r->first); }
    template <class ITRangeT>
    force_grammar(GT&, ITRangeT const&);
    
    token_type  rule_lhs(rule_type r) const
    { return gram.rule_lhs(r->first); }
    
    weight_vector const& get_weights() const {return gram.get_weights();}
    
    feature_names_type const& feature_names() const {return gram.feature_names();}
    
    feature_names_type& feature_names() {return gram.feature_names();}
    
    bool is_complete_rule(rule_type r) const 
    { return gram.is_complete_rule(r->first); }
    
    std::size_t rule_rhs_size(rule_type r) const
    { return gram.rule_rhs_size(r->first); }
    
    token_type  rule_rhs(rule_type r, std::size_t index) const
    { return gram.rule_rhs(r->first,index); }    
    
    template <class T>
    T const& rule_property(rule_type r, std::size_t index) const
    {
        return gram.template rule_property<T>(r->first,index);
    }
    
    score_t rule_score_estimate(rule_type r) const 
    { return gram.rule_score_estimate(r->first); }
    
    score_t rule_score(rule_type r) const
    { return gram.rule_score(r->first); }

    binary_rule_type const& binary_rule(rule_type r) const 
    { return gram.binary_rule(r->first); }
    
    span_string const& rule_span_string(rule_type r) const
    { return r->second; }
    
    static grammar_rule_id id(rule_type r) //const
    { return (grammar_rule_id)r; }
    
    static rule_type rule(grammar_rule_id id) //const
    { return (rule_type)id; }

    // make non-static?
    static syntax_id_type get_syntax_id(rule_type r) // const
    { return GT::get_syntax_id(r->first); }

    indexed_syntax_rule const& get_syntax(syntax_id_type id) const
    { return gram.get_syntax(id); }
    
    scored_syntax const& get_scored_syntax(rule_type r) const
    { return gram.get_scored_syntax(r->first); }
    
    scored_syntax const& get_scored_syntax(syntax_id_type id) const
    { return gram.get_scored_syntax(id); }
    
    indexed_syntax_rule const& get_syntax(rule_type r) const
    { return gram.get_syntax(r->first); }
    
    score_t get_syntax_score(syntax_id_type id) const
    { return gram.get_syntax_score(id); }

    template <class O>
    void print_syntax_rule_by_id(O&o,syntax_id_type id,bool sbtm_score=true) const 
    { gram.print_syntax_rule_by_id(o,id,sbtm_score); }

    template <class O>
    void print_syntax_rule(O&o,scored_syntax const& ss,bool sbtm_score=true) const 
    {
        gram.print_syntax_rule(o,ss,sbtm_score);
    }

    template <class O>
    void print_syntax_rule_scores(O&o,scored_syntax const& ss,bool sbtm_score=true) const 
    {
        gram.print_syntax_rule_scores(o,ss,sbtm_score);
    }
    
};

////////////////////////////////////////////////////////////////////////////////
///
///  used to force an english sentence to be translated.
///  requires that EdgeT contain as part of its aggregated info the type
///  sentence_info.
///  ie
///  \code
///  EdgeT edge;
///  sentence_info i = edge.template cast_info<sentence_info>();
///  \endcode
///  must compile.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT, class GramT, class ChartT>
class force_sentence_filter : public span_filter_interface<EdgeT,GramT,ChartT>
{
    typedef span_filter_interface<EdgeT,GramT,ChartT> base_t;
    typedef edge_filter<EdgeT> filter_type;
public:
    typedef typename GramT::rule_type rule_type;
    typedef EdgeT                                     edge_type;
    typedef edge_equivalence<EdgeT>                   edge_equiv_type;
    typedef ChartT                                    chart_type;
    typedef GramT                                     grammar_type;
    typedef typename base_t::rule_range rule_range;
    typedef typename base_t::edge_range edge_range;
    typedef typename base_t::rule_iterator rule_iterator;
    
    force_sentence_filter( substring_hash_match<indexed_token> const& match
                         , std::vector<indexed_token> const& target
                         , span_t target_span
                         , grammar_type& gram
                         , concrete_edge_factory<EdgeT,GramT>& ef
                           , chart_type& chart
                           , filter_type const& sf
        )
    : base_t(target_span,gram,ef,chart)
    , match(match) 
    , target(target)
    , filter(sf)
    , finalized(false)
    {}
    
    virtual void apply_rules( rule_range const& rr
                            , edge_range const& er1
                            , edge_range const& er2 )
    {
        if (er1.begin() == er1.end() or er2.begin() == er2.end()) return;
        span_t fc = er1.begin()->span();
        rule_iterator ri = rr.template get<0>(), re = rr.template get<1>();
        for (; ri != re; ++ri) {
            apply_rule(*ri,fc);
        }
    }
                         
    virtual void apply_rule( rule_type const& r
                           , span_t const& first_constituent );
                           
    virtual void finalize();
    virtual bool is_finalized() const;
    
    virtual edge_equiv_type const& top() const;
    virtual void pop();
    virtual bool empty() const;
    
    virtual ~force_sentence_filter() {}
private:
    boost::reference_wrapper<substring_hash_match<indexed_token> const> match;
    boost::reference_wrapper<std::vector<indexed_token> const> target;
//    edge_queue<edge_type> filter;
    filter_type filter;
    
    bool finalized;
    size_t lm_scoreable_id;
    size_t lm_string_id;
};

////////////////////////////////////////////////////////////////////////////////

class force_sentence_predicate
{
public:
    typedef bool  pred_type;
    
    force_sentence_predicate( span_t const& target_span )
    : target_span(target_span) {}

    template <class O>
    void print(O &o) const { o << "force-sentence-predicate"; }
    
    bool adjust_for_retry(unsigned i) { return i <= 1; }

    template <class E>
    pred_type keep_edge(edge_queue<E> const& pqueue, E const& e) const;
    
    template <class E>
    bool pop_top(edge_queue<E> const& pqueue) const;
private:
    span_t target_span;
    bool finalized;
};

////////////////////////////////////////////////////////////////////////////////
// typedef X pass_thru_predicate;
template <class EdgeT, class GramT, class ChartT>
class force_sentence_factory
: public span_filter_factory<EdgeT,GramT,ChartT>
{
    typedef span_filter_factory<EdgeT,GramT,ChartT> base_t;
public:
    typedef force_sentence_filter<EdgeT,GramT,ChartT>* result_type;
    
    template <class ITRangeT,class SF,class UF>
    force_sentence_factory( ITRangeT const& target
                            , span_t const& total_span
                            , SF const &sf
                            , UF const& uf
                            
        )
        : base_t(total_span)
        , target_sentence(target.begin(),target.end())
        , match(target.begin(),target.end())
        , sf(sf)
        , uf(intersect_edge_filter(force_sentence_predicate(span_t(0,target_sentence.size())),uf))
    {}
        

    virtual void print_settings(std::ostream &o) const
    {
        o << "force-sentence-factory";
    }
    
    virtual bool adjust_for_retry(unsigned i) {
        return any_change(
            sf.adjust_for_retry(i),
            uf.adjust_for_retry(i)
            );
    }  
    
    virtual result_type create( span_t const& target_span
                              , GramT& gram
                              , concrete_edge_factory<EdgeT,GramT>& ef
                              , ChartT& chart )
    {
        return new force_sentence_filter<EdgeT,GramT,ChartT>( match
                                                            , target_sentence
                                                            , target_span
                                                            , gram
                                                            , ef
                                                            , chart
                                                              , sf
            );
    }
    
    virtual edge_filter<EdgeT> unary_filter(span_t const& source_span)
    {
        return uf;
        //make_predicate_edge_filter<EdgeT>(  force_sentence_predicate(span_t(0,target_sentence.size()))                );
    }
    
    virtual ~force_sentence_factory(){}
    
private:
    std::vector<indexed_token> target_sentence;
    substring_hash_match<indexed_token> match;
    edge_filter<EdgeT> sf,uf;
};

////////////////////////////////////////////////////////////////////////////////



} // namespace sbmt

# include <sbmt/search/impl/force_sentence_filter.ipp>

# endif //     SBMT__SEARCH__FORCE_SENTENCE_FILTER_HPP

