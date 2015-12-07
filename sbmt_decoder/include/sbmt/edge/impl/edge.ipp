// sbmt/edge/impl/edge.ipp

#include <sbmt/edge/edge_equivalence.hpp>
#include <sbmt/edge/info_combining.hpp>
#include <sbmt/hash/ref_array.hpp>
#include <boost/functional/hash/hash.hpp>
#include <sbmt/logging.hpp>
#ifdef max
#undef max
#endif



#include <limits>

namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(edge_domain,"edge",root_domain);

template <class O,class InfoT,class TF>
void print(O& out, edge<InfoT> const& e, TF const& tf)
{
#if 0
    out << print(e.root(),tf) 
        << ":" << print(e.info(),tf) 
        << ":scr=" << e.score();
    if (e.has_first_child()) {
        out << ":" << e.first_child().span() << ":"
            << print(e.first_child().root(),tf) 
            << ":" << print(e.first_child().info(),tf)
            << "scr=" << e.first_child().score();
    }
    if (e.has_second_child()) {
        out << ":" << e.second_child().span() << ":"
            << print(e.second_child().root(),tf) 
            << ":" << print(e.second_child().info(),tf)
            << "scr=" << e.second_child().score();
    }
#else
    e.print_state(out,tf);
    out << ":p="<<e.inside_score()<<":score="<<e.score();
#endif 
}

template <class InfoFactory>
class edge_generator {
    typedef typename InfoFactory::info_type info_type;
    typedef typename info_result_generator<InfoFactory>::type generator_type;
    typedef typename edge<info_type>::equiv_p equiv_p;
public:
    typedef edge<info_type> result_type;
    
    operator bool() const { return bool(generator); }
    
    result_type operator()()
    {
        info_type info;
        score_t inside, heuristic;
        boost::tie(info,inside,heuristic) = generator();
        inside *= tm_inside;
        heuristic *= tm_heuristic;
        ++(ef->num_edges);
        
        edge<info_type> e(root,rule_id,info,inside,heuristic,e1,e2);
        SBMT_PEDANTIC_MSG(
          edge_domain
        , "edge: %s tmheur: %s heur: %s tmscr: %s scr: %s"
        , ef->hash_string(*grammar,e) 
        % tm_heuristic
        % heuristic
        % tm_inside
        % inside
        )
        ;
        return e;
    }
    template <class Grammar>
    edge_generator( edge_factory<InfoFactory>* ef
                  , Grammar& grammar
                  , typename Grammar::rule_type rule
                  , generator_type const& generator 
                  , score_t cell_h
                  , equiv_p e1
                  , equiv_p e2 ) 
    : grammar(&grammar)
    , ef(ef)
    , generator(generator)
    , tm_inside(grammar.rule_score(rule))
    , tm_heuristic(grammar.rule_score_estimate(rule) * cell_h)
    , rule_id(grammar.id(rule))
    , root(grammar.rule_lhs(rule))
    , e1(e1)
    , e2(e2) {}
private:
    grammar_in_mem const* grammar;
    edge_factory<InfoFactory>* ef;
    generator_type generator;
    score_t tm_inside;
    score_t tm_heuristic;
    grammar_rule_id rule_id;
    indexed_token root;
    equiv_p e1;
    equiv_p e2;
};

////////////////////////////////////////////////////////////////////////////////

template <class IF>
template <class GT>
edge_generator<IF>
edge_factory<IF>::create_edge( GT const& grammar
                             , typename GT::rule_type r
                             , edge_equiv_type const& e1
                             , edge_equiv_type const& e2 )
{
    assert(grammar.rule_rhs(r,0) == e1.representative().root());
    assert(grammar.rule_rhs(r,1) == e2.representative().root());
    assert(grammar.rule_rhs_size(r) == 2);
    score_t cell_h = get_cell_heuristic(grammar.rule_lhs(r), combine(e1.span(),e2.span())); 

    return edge_generator<IF>(
        this
      , grammar
      , r
      , sbmt::create_info( *info_factory
                         , grammar
                         , r
                         , combine(e1.representative().span(), e2.representative().span())
                         , cref_array(e1.representative(), e2.representative()) 
                         )
      , cell_h
      , e1.get_shared()
      , e2.get_shared()
    )
    ;
}

////////////////////////////////////////////////////////////////////////////////

template <class IF>
template <class GT>
edge_generator<IF>
edge_factory<IF>::create_edge( GT const& grammar
                             , typename GT::rule_type r
                             , edge_equiv_type const& e )
{
    assert(grammar.rule_rhs(r,0) == e.representative().root());
    assert(grammar.rule_rhs_size(r) == 1);
    
    score_t cell_h = get_cell_heuristic(grammar.rule_lhs(r), e.span());
    
    return edge_generator<IF>(
        this
      , grammar
      , r
      , create_info( *info_factory
                   , grammar
                   , r
                   , e.representative().span()
                   , cref_array(e.representative())
                   )
      , cell_h
      , e.get_shared()
      , NULL
    )
    ;  
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
span_t const& edge<IT>::span() const
{
    return spn;
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
inline unsigned edge<IT>::child_count() const
{
    if (!has_first_child()) return 0;
    if (!has_second_child()) return 1;
    return 2;
}

template <class IT>
inline bool edge<IT>::has_first_child() const
{
    return child[0] && !child[0]->empty();
}

template <class IT>
inline bool edge<IT>::has_second_child() const
{
    return child[1];
}

template <class IT>
bool edge<IT>::is_unary() const
{
    return has_first_child() and !has_second_child();
}

template <class IT>
bool edge<IT>::is_binary() const
{
    return has_second_child();
}

////////////////////////////////////////////////////////////////////////////////
/// 
/// omit_span is used internally so that recursive checks of non-scoreable edges
/// do not include it in determining equality
///
///////////////////////////////////////////////////////////////////////////////
template <class IT>
bool edge<IT>::equal_to(edge const& other) const
{
    
    return rt == other.rt and
           span() == other.span() and
           (rt.type() == top_token ? true : edge_info_equal(*this,other));
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
std::size_t edge<IT>::hash_value() const
{   
    std::size_t retval = boost::hash<indexed_token>()(rt); 
    boost::hash_combine(retval, span());
    if (rt.type() != top_token)
        boost::hash_combine(retval,edge_info_hash(*this));
    return retval;
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
typename edge<IT>::edge_equiv_type edge<IT>::first_children() const
{
    assert (child[0] and !child[0]->empty());
    return edge_equiv_type(child[0]);
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
typename edge<IT>::edge_equiv_type edge<IT>::second_children() const
{
    assert (child[1] and !child[1]->empty());
    return edge_equiv_type(child[1]);
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
typename edge<IT>::edge_type const& edge<IT>::first_child() const
{
    assert (child[0] and !child[0]->empty());
    return child[0]->representative();
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
typename edge<IT>::edge_type const& edge<IT>::second_child() const
{
    assert (child[1] and !child[1]->empty());
    return child[1]->representative();
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
typename edge<IT>::edge_equiv_type edge<IT>::get_children(unsigned i) const
{
    assert (child[i] and !child[i]->empty());
    return edge_equiv_type(child[i]);
}

template <class IT>
typename edge<IT>::edge_type const& edge<IT>::get_child(unsigned i) const
{
    assert (child[i] and !child[i]->empty());
    return child[i]->representative();
}


template <class IT>
typename edge<IT>::edge_type & edge<IT>::get_lmchild(unsigned i) const
{
    if (i==1) {
        assert(get_child(0).root().type()!=foreign_token);
        return get_child(1);
    }
    
    edge_type &c0=get_child(0);
    assert (i==0);
    if (c0.root().type()==foreign_token)
        return get_child(1);
    else
        return c0;
}

////////////////////////////////////////////////////////////////////////////////

template <class IT>
void edge<IT>::adjust_child(edge_equiv_type new_child,unsigned child_i)
{
    edge_type const& old_edge=get_child(child_i);
    child[child_i]=new_child.get_shared();
    edge_type const& new_edge=get_child(child_i);
    //now: new_edge==new_child.representative();
    assert(old_edge.equal_to(new_edge)); // equivalent, or else error

    score_t adjust = new_edge.inside / old_edge.inside;
    // add new child inside cost    
    // remove old child inside cost
    inside*=adjust;
    total*=adjust;
}

////////////////////////////////////////////////////////////////////////////////

template <class InfoFactory>
template <class Grammar, class Stop>
void
edge_factory<InfoFactory>::component_info_scores( edge<info_type> const& e
                                                , Grammar& grammar
                                                , feature_vector& scorevec
                                                , feature_vector& heurvec 
                                                , Stop const& stop ) const
{
    scorevec.clear();
    component_scores(*info_factory, grammar, e, scorevec, heurvec, stop);
}

template <class Edge> 
edge_factory<Edge>::edge_factory( info_factory_p info_factory
                                , double tm_heuristic_scale // tm_wt * (inside + tm_heuristic_scale*heuristic)
                                , double info_heuristic_scale // likewise

    )
: tm_wt(1.0)
, info_wt(1.0)
, info_factory(info_factory)
, cells(0)
, tm_h_wt(tm_wt*tm_heuristic_scale)
, info_h_wt(info_wt*info_heuristic_scale)
, num_edges(0)
{assert_1_wt();}

template <class InfoFactory> 
edge_factory<InfoFactory>::edge_factory(InfoFactory const& ifact)
  : tm_wt(1.0)
  , info_wt(1.0)
  , info_factory(new InfoFactory(ifact))
  , cells(0)
  , tm_h_wt(1.0)
  , info_h_wt(1.0)
  , num_edges(0)
  {assert_1_wt();}

////////////////////////////////////////////////////////////////////////////////

template <class IFT> 
edge_factory<IFT>::edge_factory(
    double tm_heuristic_scale // tm_wt * (inside + tm_heuristic_scale*heuristic)
    , double info_heuristic_scale // likewise
    )
: tm_wt(1.0)
, info_wt(1.0)
, info_factory(new info_factory_type())
, cells(0)
, tm_h_wt(tm_wt*tm_heuristic_scale)
, info_h_wt(info_wt*info_heuristic_scale)
, num_edges(0)
{assert_1_wt();}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
