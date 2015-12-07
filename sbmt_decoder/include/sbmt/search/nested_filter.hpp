# if ! defined(SBMT__SEARCH__NESTED_FILTER_HPP)
# define       SBMT__SEARCH__NESTED_FILTER_HPP

# include <sbmt/search/span_filter_interface.hpp>
# include <boost/scoped_ptr.hpp>

namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////
///  
///  often a filter you want to write just wraps an existing filter, perhaps
///  intercepting one or two calls.  this is a convenience class for deriving
///  such a filter
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT, class GramT, class ChartT>
class nested_filter
 : public span_filter_interface<EdgeT,GramT,ChartT>
{
public:
    typedef span_filter_interface<EdgeT,GramT,ChartT> base_t;
    
    // \note: you don't own filter after you pass it in.
    // auto_ptr makes this explicit and avoids dangerous ambiguity if exception
    // occurs during construction.
    nested_filter( std::auto_ptr<base_t> filter  
                 , span_t const& target_span 
                 , typename base_t::grammar_type& gram
                 , concrete_edge_factory<EdgeT,GramT>& ef
                 , typename base_t::chart_type& chart )
      : base_t(target_span,gram,ef,chart)
      , filter(filter) {}
                         
    virtual void apply_rules( typename base_t::rule_range const& rr
                            , typename base_t::edge_range const& er1
                            , typename base_t::edge_range const& er2 )
    { filter->apply_rules(rr,er1,er2); }
    virtual void finalize() { filter->finalize(); }
    virtual bool is_finalized() const { return filter->is_finalized(); }   
    virtual typename base_t::edge_equiv_type const& top() const
    { return filter->top(); }
    virtual void pop() { filter->pop(); }
    virtual bool empty() const { return filter->empty(); }
    
    virtual ~nested_filter() {}
protected:
    boost::scoped_ptr<base_t> filter;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class nested_filter_factory
 : public span_filter_factory<EdgeT,GramT,ChartT>
{
public:
    typedef span_filter_factory<EdgeT,GramT,ChartT> base_t;
    typedef span_filter_interface<EdgeT,GramT,ChartT>  base_filter_type;
    typedef base_filter_type* result_type;
    
    nested_filter_factory( boost::shared_ptr<base_t> factory
                         , span_t const& total_span )
    : base_t(total_span)
    , factory(factory) {}

    virtual void print_settings(std::ostream &o) const
    { 
        factory->print_settings(o);
    }
    
    virtual bool adjust_for_retry(unsigned retry_i) 
    { return factory->adjust_for_retry(retry_i); }  
    
    virtual result_type create( span_t const& target_span
                              , GramT& gram
                              , concrete_edge_factory<EdgeT,GramT>& ecs
                              , ChartT& chart ) 
    {
        return factory->create(target_span,gram,ecs,chart);
    }
                              
    virtual edge_filter<EdgeT> unary_filter(span_t const& target_span) 
    {
        return factory->unary_filter(target_span);
    }
    
    virtual ~nested_filter_factory(){}
    
protected:
    boost::shared_ptr<base_t> factory;
};

////////////////////////////////////////////////////////////////////////////////
    
} // namespace sbmt

# endif //     SBMT__SEARCH__NESTED_FILTER_HPP
