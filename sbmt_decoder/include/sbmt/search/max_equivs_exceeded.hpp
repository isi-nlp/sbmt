# if ! defined(SBMT__SEARCH__MAX_EDGES_EXCEEDED_HPP)
# define       SBMT__SEARCH__MAX_EDGES_EXCEEDED_HPP

# include <sbmt/search/nested_filter.hpp>
# include <stdexcept>
# include <limits>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

class max_edges_exceeded : public std::runtime_error {
public:
    // changing this message can break compatibility with scripts that read logs
    max_edges_exceeded() 
      : std::runtime_error("maximum edge count exceeded") {}
};

class max_equivs_exceeded : public std::runtime_error {
public:
    // changing this message can break compatibility with scripts that read logs
    max_equivs_exceeded() 
      : std::runtime_error("maximum edge-equivalence count exceeded") {}
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
class max_edges_exceeded_filter : public nested_filter<ET, GT, CT>
{
    typedef nested_filter<ET, GT, CT> base_t;
    std::size_t max_equivs;
    std::size_t max_edges;
    concrete_edge_factory<ET,GT>& ef;
public:
    max_edges_exceeded_filter(
        std::auto_ptr< span_filter_interface<ET,GT,CT> > filter
      , span_t const& target_span 
      , typename base_t::grammar_type& gram
      , concrete_edge_factory<ET,GT>& ef
      , typename base_t::chart_type& chart
      , std::size_t max_equivs
      , std::size_t max_edges
    )
    : base_t(filter,target_span,gram,ef,chart)
    , max_equivs(max_equivs)
    , max_edges(max_edges)
    , ef(ef) {}
    
    virtual void apply_rules( typename base_t::rule_range const& rr
                            , typename base_t::edge_range const& er1
                            , typename base_t::edge_range const& er2 )
    { 
        edge_stats estats = ef.stats();
        if (estats.edge_equivalences() > max_equivs) {
            throw max_equivs_exceeded();
        } else if (estats.edges() > max_edges) {
            throw max_edges_exceeded();
        } else base_t::filter->apply_rules(rr,er1,er2); 
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
class max_edges_exceeded_factory : public nested_filter_factory<ET,GT,CT>
{
    typedef span_filter_factory<ET,GT,CT>        factory_t;
    typedef nested_filter_factory<ET,GT,CT>      base_t;
    typedef span_filter_interface<ET,GT,CT>      base_filter_t;
    typedef max_edges_exceeded_filter<ET,GT,CT> filter_t;
    typedef base_filter_t*                       result_type;
    
    std::size_t max_equivs;
    std::size_t max_edges;
public:
    max_edges_exceeded_factory( 
        boost::shared_ptr<factory_t> factory
      , span_t const& total_span
      , std::size_t max_equivs
      , std::size_t max_edges
    ) 
    :  base_t(factory,total_span)
    ,  max_equivs(max_equivs)
    ,  max_edges(max_edges) {}
    
    virtual result_type create( span_t const& target_span
                              , GT& gram
                              , concrete_edge_factory<ET,GT>& ecs
                              , CT& chart ) 
    {
        std::auto_ptr<base_filter_t> 
            p(base_t::create(target_span,gram,ecs,chart));
        return new filter_t(p,target_span,gram,ecs,chart,max_equivs,max_edges);
    }
};

} // namespace sbmt

# endif //     SBMT__SEARCH__MAX_EDGES_EXCEEDED_HPP
