#ifndef   SBMT_SEARCH_SPAN_FILTER_HPP
#define   SBMT_SEARCH_SPAN_FILTER_HPP

#include <sbmt/edge/edge.hpp>
#include <sbmt/chart/chart.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/search/edge_filter.hpp>
#include <sbmt/search/span_filter_interface.hpp>


namespace sbmt {

template <class EdgeT, class GramT, class ChartT>
class exhaustive_span_filter
: public span_filter_interface<EdgeT, GramT, ChartT>
{
public:
    typedef span_filter_interface<EdgeT, GramT, ChartT> base_t;
private:
    typedef typename base_t::rule_range rule_range_;
	typedef typename base_t::rule_type rule_type_;
    typedef typename base_t::edge_range edge_range_;
	typedef typename base_t::edge_equiv_type edge_equiv_;
public:
    template <class SpanFilterFunc>
    exhaustive_span_filter( SpanFilterFunc sf
                          , span_t const& target_span
                          , GramT& gram
                          , concrete_edge_factory<EdgeT,GramT>& ecs
                          , ChartT& chart );
               
    virtual
    void apply_rules( rule_range_ const& rr
                    , edge_range_ const& er1
                    , edge_range_ const& er2 );
                            
    void apply_rule( rule_type_ const& r
                   , edge_range_ const& er1
                   , edge_range_ const& er2
                   );
                   
    virtual void finalize();
    virtual bool is_finalized() const;
    
    virtual void pop();
	virtual typename base_t::edge_equiv_type const& top() const 
	{
		 return filter.top();
	}
    virtual bool empty() const;
    
    virtual ~exhaustive_span_filter() {}
    
private:
    edge_filter<EdgeT> filter;
};

template <class EdgeT, class GramT, class ChartT>
class exhaustive_span_factory
: public span_filter_factory<EdgeT,GramT,ChartT>
{
public:
    typedef span_filter_factory<EdgeT,GramT,ChartT> base_t;
    typedef exhaustive_span_filter<EdgeT,GramT,ChartT> filter_t;
    
    template <class FilterT>
    exhaustive_span_factory( FilterT sf
                           , span_t const& total_span )
    : base_t(total_span)
    , sf(sf)
    , uf(sf) {}
    
    template <class FilterT, class UnaryFilterF>
    exhaustive_span_factory( FilterT sf
                           , UnaryFilterF uf
                           , span_t const& total_span )
    : base_t(total_span)
    , sf(sf)
    , uf(uf) {}

    template <class O>
    void print(O &o) const
    {
        o << "exhaustive_span_factory{ ";
        sf.print(o);
        o << " }";
    }
    
    virtual edge_filter<EdgeT> unary_filter(span_t const& target_span)
    {
        return uf;
    }

    virtual void print_settings(std::ostream &o) const 
    { print(o); }
    
    virtual bool adjust_for_retry(unsigned i) 
    { 
        return any_change( sf.adjust_for_retry(i)
                         , uf.adjust_for_retry(i) ); 
    }
    
    virtual typename base_t::result_type
    create( span_t const& target_span 
          , GramT& gram
          , concrete_edge_factory<EdgeT,GramT>& ef
          , ChartT& chart )
    {
        return new filter_t(sf,target_span,gram,ef,chart);
    }
    
    virtual ~exhaustive_span_factory(){}
private:
    edge_filter<EdgeT> sf;
    edge_filter<EdgeT> uf;
};

} // namespace sbmt

#include <sbmt/search/impl/span_filter.ipp>

#endif // SBMT_SEARCH_SPAN_FILTER_HPP
