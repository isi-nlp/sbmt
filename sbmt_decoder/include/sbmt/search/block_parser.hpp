# if ! defined(SBMT__SEARCH__BLOCK_PARSER_HPP)
# define       SBMT__SEARCH__BLOCK_PARSER_HPP
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/span.hpp>
# include <sbmt/token.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/grammar/lm_string.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/search/span_filter_interface.hpp>
# include <sbmt/search/unary_filter_interface.hpp>
# include <sbmt/search/filter_bank.hpp>
# include <sbmt/search/cky_logging.hpp>
# include <sbmt/search/negative_block_filter.hpp>

# include <map>
# include <string>
# include <list>
# include <iostream>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

bool order_preserving(indexed_lm_string const& lmstr);

template <class Gram>
struct order_preserve_filt {
    order_preserve_filt(Gram const& gram)
      : gram(&gram) {}
    bool operator()(typename Gram::rule_type const& r) const
    {
        return false; // we need a new way to calculate... 
                      // order_preserving(gram->rule_lm_string(r));
    }
    Gram const* gram;
};

template <class Gram> order_preserve_filt<Gram>
make_order_preserve_filt(Gram const& gram)
{
    return order_preserve_filt<Gram>(gram);
}

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class block_parser_filter
: public span_filter_interface<EdgeT,GramT,ChartT>
{
public:
    typedef span_filter_interface<EdgeT,GramT,ChartT> span_filter_t;
    typedef span_filter_t base_t;
private:
    typedef typename base_t::rule_range rule_range_;
	typedef typename base_t::rule_type rule_type_;
    typedef typename base_t::edge_range edge_range_;
	typedef typename base_t::edge_equiv_type edge_equiv_;
public:
    
    block_parser_filter( boost::shared_ptr<span_filter_t> filter
                       , bool preserve_order
                       , span_t const& target_span
                       , GramT& gram
                       , concrete_edge_factory<EdgeT,GramT>& ef
                       , ChartT& chart );
                            
    virtual void apply_rules( rule_range_ const& rr
                            , edge_range_ const& er1
                            , edge_range_ const& er2 );
                                                 
    virtual void finalize() { filter->finalize(); }
    
    virtual bool is_finalized() const { return filter->is_finalized(); }
    
    virtual typename base_t::edge_equiv_type const& top() const 
    { return filter->top(); }
    
    virtual bool empty() const { return filter->empty(); }
    
    virtual void pop() { filter->pop(); }
    
    virtual ~block_parser_filter() {}
private:
    boost::shared_ptr<span_filter_t> filter;
    bool preserve_order;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT>
class block_parser_unary_filter
: public nested_unary_filter<EdgeT,GramT>
{
public:
    typedef unary_filter_interface<EdgeT,GramT> unary_filter_;
    typedef nested_unary_filter<EdgeT,GramT> base_;
private:
    typedef typename base_::rule_range rule_range_;
	typedef typename base_::edge_equiv_type edge_equiv_;
public:
    block_parser_unary_filter( std::auto_ptr<unary_filter_> filter
                             , bool preserve_order
                             , span_t const& target_span
                             , GramT& gram
                             , concrete_edge_factory<EdgeT,GramT>& ef )
      : base_(filter,ef,gram,target_span)
      , preserve_order(preserve_order) {}
    virtual void apply_rules( edge_equiv_ source
                            , rule_range_ rr );
private:
    bool preserve_order;
};

template <class EdgeT, class GramT>
class block_parser_unary_filter_factory
: public unary_filter_factory<EdgeT,GramT> {
public:
    typedef unary_filter_factory<EdgeT,GramT> unary_filter_factory_;
    
    virtual bool adjust_for_retry(unsigned retry_i)
    {
        return factory->adjust_for_retry(retry_i);
    }
    
    block_parser_unary_filter_factory(
                           boost::shared_ptr<unary_filter_factory_> factory
                         , bool preserve_order
                         , span_t const& total_span 
                    ) 
    : unary_filter_factory_(total_span)
    , factory(factory)
    , preserve_order(preserve_order) {}
    
    virtual 
    typename unary_filter_factory_::result_type create( span_t const& target_span
                                                      , GramT& gram
                                                      , concrete_edge_factory<EdgeT,GramT>& ef )
    {
        typedef unary_filter_interface<EdgeT,GramT> unary_filter_;
        std::auto_ptr<unary_filter_> p(factory->create(target_span,gram,ef));
        return typename unary_filter_factory_::result_type(
                   new block_parser_unary_filter<EdgeT,GramT> (
                         p
                       , preserve_order
                       , target_span
                       , gram
                       , ef ) 
              );
    }
protected:
    boost::shared_ptr<unary_filter_factory_> factory;
    bool preserve_order;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class block_parser_filter_factory
: public span_filter_factory<EdgeT,GramT,ChartT>
{
public:
    typedef span_filter_factory<EdgeT,GramT,ChartT> base_t;
    typedef span_filter_factory<EdgeT,GramT,ChartT> span_filter_factory_t;
    
    virtual void print_settings(std::ostream &o) const 
    {
        o << "block_parser_filter_factory:factory=";
        factory->print_settings(o);
    }
    virtual bool adjust_for_retry(unsigned retry_i)
    {
        return factory->adjust_for_retry(retry_i);
    }
    block_parser_filter_factory(
                           boost::shared_ptr<span_filter_factory_t> factory
                         , bool preserve_order
                         , span_t const& total_span 
                    ) 
    : base_t(total_span)
    , factory(factory)
    , preserve_order(preserve_order) {}
    
    virtual 
    typename base_t::result_type create( span_t const& target_span
                                       , GramT& gram
                                       , concrete_edge_factory<EdgeT,GramT>& ecs
                                       , ChartT& chart )
    {
        typedef span_filter_interface<EdgeT,GramT,ChartT> span_filter_t;
    
        boost::shared_ptr<span_filter_t> 
            filt(factory->create(target_span,gram,ecs,chart));
        
        return new block_parser_filter<EdgeT,GramT,ChartT> (
                                                        filt
                                                      , preserve_order
                                                      , target_span
                                                      , gram
                                                      , ecs
                                                      , chart );
    }
    
    virtual edge_filter<EdgeT> unary_filter(span_t const& target_span)
    {
        return factory->unary_filter(target_span);
    }

private:
    boost::shared_ptr<span_filter_factory_t> factory;
    bool preserve_order;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class transform_filter_bank
{
public:
    typedef EdgeT edge_type;
    typedef edge_equivalence<EdgeT> edge_equiv_type;
    typedef span_filter_factory<EdgeT,GramT,ChartT>   span_filter_factory_t;
    typedef boost::shared_ptr<span_filter_factory_t>  span_filter_factory_p;
    typedef unary_filter_factory<EdgeT,GramT>         unary_filter_factory_t;
    typedef boost::shared_ptr<unary_filter_factory_t> unary_filter_factory_p;
    
    transform_filter_bank( span_filter_factory_p span_filt_factory
                         , unary_filter_factory_p unary_filt_factory
                         , bool preserve_order
                         , span_transform const& transform
                         , GramT& gram
                         , concrete_edge_factory<EdgeT,GramT> &ef
                         , edge_equivalence_pool<EdgeT>& epool
                         , ChartT& chart
                         , span_t const& total_span )
    : fb(
            span_filter_factory_p(
              new block_parser_filter_factory<EdgeT,GramT,ChartT>( 
                      span_filt_factory
                    , preserve_order
                    , total_span
                )
            )
          , unary_filter_factory_p(
              new block_parser_unary_filter_factory<EdgeT,GramT>(
                      unary_filt_factory
                    , preserve_order
                    , total_span
                  )
            )
          , gram
          , ef
          , epool
          , chart
          , total_span
      ) 
    , transform(transform) {} 

    void apply_rules(span_t const& first_constit, span_index_t right_boundary)
    { fb.apply_rules(transform(first_constit), transform(right_boundary)); }
    
    void finalize(span_t const& s)
    { fb.finalize(transform(s)); }
    
    edge_stats ecs_stats() const
    { return fb.ecs_stats(); }
    
    span_filter_factory_p get_filter_factory() 
    { return fb.get_filter_factory(); }
    
    ChartT& get_chart() { return fb.get_chart(); }
    GramT&  get_grammar() { return fb.get_grammar(); }
    
private:
    filter_bank<EdgeT,GramT,ChartT> fb;
    span_transform transform;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
block_parser_filter<ET,GT,CT>::block_parser_filter( 
                                          boost::shared_ptr<span_filter_t> filt
                                        , bool preserve_order
                                        , span_t const& target_span
                                        , GT& gram
                                        , concrete_edge_factory<ET,GT>& ef
                                        , CT& chart )
: base_t(target_span,gram,ef,chart)
, filter(filt)
, preserve_order(preserve_order)
{}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT> void 
block_parser_filter<ET,GT,CT>::apply_rules( 
                                 rule_range_ const& rr
                               , edge_range_ const& er1
                               , edge_range_ const& er2 
                               )
{
    if (!preserve_order) filter->apply_rules(rr,er1,er2);
    else {
        using namespace boost;
        typename base_t::rule_iterator ritr, rend;
        tie(ritr,rend) = rr;
        
        filter->apply_rules(
            make_tuple(
                make_filter_iterator(
                    make_order_preserve_filt(base_t::gram)
                  , ritr
                  , rend
                )
              , make_filter_iterator(
                    make_order_preserve_filt(base_t::gram)
                  , rend
                  , rend
                )
            )
          , er1
          , er2
        );
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT> void 
block_parser_unary_filter<ET,GT>::apply_rules( 
                                edge_equiv_ source
                              , rule_range_ rr 
                            )
{
    if (!preserve_order) base_::filter->apply_rules(source,rr);
    else {
        using namespace boost;
        typename base_::rule_iterator ritr, rend;
        tie(ritr,rend) = rr;
        
        base_::filter->apply_rules(
            source
          , make_tuple(
                make_filter_iterator(
                    make_order_preserve_filt(unary_filter_::gram)
                  , ritr
                  , rend
                )
              , make_filter_iterator(
                    make_order_preserve_filt(unary_filter_::gram)
                  , rend
                  , rend
                )
            )
        );
    }
}

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

template <class GT>
struct block_lattice_chart
{
    template <class CT, class ET>
    void operator()( CT &chart
                   , concrete_edge_factory<ET,GT>& ef
                   , edge_equivalence_pool<ET>& epool ) const;
                   
    block_lattice_chart(lattice_tree const& ltree, GT& gram) 
    : ltree(ltree), gram(gram) {}
private:
    lattice_tree ltree;
    GT& gram;
    
    template <class CT, class ET>
    void init_chart( CT& chart
                   , concrete_edge_factory<ET,GT>& ef
                   , edge_equivalence_pool<ET>& epool
                   , lattice_tree::node nd ) const;
};

template <class GT>
template <class CT, class ET>
void block_lattice_chart<GT>::init_chart( CT& chart
                                        , concrete_edge_factory<ET,GT>& ef
                                        , edge_equivalence_pool<ET>& epool
                                        , lattice_tree::node nd ) const
{
    if (!nd.is_internal()) return;
    lattice_tree::children_iterator itr = nd.children_begin(),
                                    end = nd.children_end();
    
    for (; itr != end; ++itr) {
        if (itr->is_internal()) {
            init_chart(chart,ef,epool,*itr);
        } else {
            typename GT::rule_type br;
            ET f = ef.create_edge( itr->lat_edge().source
                                 , itr->span()
                                 , 1.0 );
            edge_equivalence<ET> fq(f);
            br = gram.rule(itr->lat_edge().rule_id);
            gusc::any_generator<ET> gen = ef.create_edge(gram, br, fq);
            
            if (itr->lat_edge().syntax_rule_id == NULL_GRAMMAR_RULE_ID) {
                while (gen) chart.insert_edge(epool,gen());
            } else {
                edge_queue<ET> edges;
                br = gram.rule(itr->lat_edge().syntax_rule_id);
                while (gen) edges.insert(epool,gen());
                while (not edges.empty()) {
                    gusc::any_generator<ET> 
                        gen2 = ef.create_edge(gram,br,edges.top());
                    edges.pop();
                    while (gen2) chart.insert_edge(epool,gen2());
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
template <class CT, class ET> void
block_lattice_chart<GT>::operator()( CT& chart
                                   , concrete_edge_factory<ET,GT>& ef
                                   , edge_equivalence_pool<ET>& epool ) const
{
    chart.reset(ltree.root().span().right());
    init_chart(chart,ef,epool,ltree.root());
}

////////////////////////////////////////////////////////////////////////////////

template <class PF>
class block_parse_cky {
public:
    block_parse_cky(lattice_tree const& ltree, PF parse_func)
    : ltree(ltree)
    , parse_func(parse_func) {}
    
    template <class FBT> 
    void operator()( FBT& filter_bank
                   , span_t target_span
                   , cky_generator const& cky_gen = full_cky_generator() 
                   ) const;
private:
    template <class FBT>
    void parse( lattice_tree::node const& n
              , FBT& fbt
              , boost::shared_ptr<
                    span_filter_factory<
                           typename FBT::edge_type
                         , typename FBT::gram_type
                         , typename FBT::chart_type
                    >
                > sfact
              , span_t const& total_span) const;
    std::set<span_t>& compute_blocks( lattice_tree::node const& n
                                    , std::set<span_t>& ) const;
    
    lattice_tree ltree;
    PF parse_func;
};

////////////////////////////////////////////////////////////////////////////////

template <class PF>
std::set<span_t>& 
block_parse_cky<PF>::compute_blocks( lattice_tree::node const& n
                                   , std::set<span_t>& blocks ) const
{
    if (n.is_internal()) {
        blocks.insert(n.span());
        lattice_tree::children_iterator 
            itr = n.children_begin(),
            end = n.children_end();
        for (; itr != end; ++itr) {
            compute_blocks(*itr,blocks);
        }
    }
    return blocks;
}

////////////////////////////////////////////////////////////////////////////////

template <class PF>
template <class FBT> void
block_parse_cky<PF>::operator()( FBT& filter_bank
                               , span_t total_span
                               , cky_generator const& cky_gen ) const
{
    lattice_tree::node root = ltree.root();
    boost::shared_ptr< 
        span_filter_factory< 
            typename FBT::edge_type
          , typename FBT::gram_type
          , typename FBT::chart_type 
        > 
    > sfilt = filter_bank.get_filter_factory();
    
    if (ltree.has_span_restrictions()) {
        typedef negative_block_filter_factory< 
                  typename FBT::edge_type
                , typename FBT::gram_type
                , typename FBT::chart_type > filter_t;

        sfilt.reset(new filter_t(sfilt, total_span, ltree.span_restrictions()));
    }
    
    parse(root, filter_bank, sfilt, total_span);

}

////////////////////////////////////////////////////////////////////////////////

template <class Chart>
void remove_incomplete_edges(Chart& chart, span_t const& spn)
{
    std::vector<typename Chart::edge_equiv_type> keepers;
    typename Chart::cell_range cells = chart.cells(spn);
    typename Chart::cell_iterator ci = cells.begin(), ce = cells.end();
    size_t kept_cells = 0;
    size_t rejected_cells = 0;
    for (; ci != ce; ++ci) {
        if (is_native_tag(ci->root())) {
            ++kept_cells;
            typename Chart::edge_range edges = ci->edges();
            typename Chart::edge_iterator ei = edges.begin(), ee = edges.end();
            for (; ei != ee; ++ei) {
                keepers.push_back(*ei);
            }
        } else {
            ++rejected_cells;
        }
    }
    chart.clear(spn);
    typename std::vector<typename Chart::edge_equiv_type>::iterator 
        i = keepers.begin(),
        e = keepers.end();
    for (; i != e; ++i) chart.insert_edge(*i);
    SBMT_DEBUG_MSG(
        cky_domain
      , "remove_incomplete_edges: kept %s rejected %s for block %s"
      , kept_cells % rejected_cells % spn
    );
      
}

template <class PF>
template <class FBT>
void block_parse_cky<PF>::parse( lattice_tree::node const& n
                               , FBT& sfb
                               , boost::shared_ptr<
                                     span_filter_factory<
                                         typename FBT::edge_type
                                       , typename FBT::gram_type
                                       , typename FBT::chart_type
                                     >
                                 > sfact
                               , span_t const& total_span ) const
{
    if (!n.is_internal()) return;
    lattice_tree::children_iterator itr = n.children_begin(),
                                    end = n.children_end();
    std::set<span_index_t> sset;
    for (; itr != end; ++itr) {
        parse(*itr,sfb,sfact,itr->span());
        sset.insert(itr->span().left());
        sset.insert(itr->span().right());
    }
    
    span_transform transform(sset.begin(),sset.end());
    
    typedef transform_filter_bank< typename FBT::edge_type
                                 , typename FBT::gram_type
                                 , typename FBT::chart_type > filter_bank_t;
    
    filter_bank_t fb( sfact
                    , sfb.get_unary_filter_factory()
                    , false
                    , transform
                    , sfb.get_grammar()
                    , sfb.get_edge_factory()
                    , sfb.get_edge_pool()
                    , sfb.get_chart()
                    , total_span );
    SBMT_INFO_STREAM(cky_domain, "parsing block: " << n.span());
    parse_func(fb, span_t(0,sset.size() - 1));
    remove_incomplete_edges(sfb.get_chart(),total_span);
    SBMT_INFO_STREAM(cky_domain, "block:" << n.span() << " complete");
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__SEARCH__BLOCK_PARSER_HPP
