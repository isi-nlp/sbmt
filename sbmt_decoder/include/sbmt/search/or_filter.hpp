# if ! defined(SBMT__SEARCH__OR_FILTER_HPP)
# define       SBMT__SEARCH__OR_FILTER_HPP

# include <boost/shared_ptr.hpp>
# include <sbmt/search/span_filter_interface.hpp>
# include <sbmt/grammar/grammar.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  divert rule application to one of two span-filters, depending on the 
///  results of a predicate-function pred(gram,rule)
///  the type of span_filters used is arbitrary and independent.
///
///  the predicate-function must be such that unary rules are always true.
///  the predicate-function should be a functor that additionally has a 
///  string label() const method
///
////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT, class BoolFunction>
class or_filter
: public span_filter_interface<EdgeT,GramT,ChartT>
{
public:
    typedef span_filter_interface<EdgeT,GramT,ChartT> span_filter_t;
    typedef span_filter_interface<EdgeT,GramT,ChartT> base_t;
    typedef BoolFunction predicate_type;
private:
    typedef typename base_t::rule_range rule_range_;
    typedef typename base_t::edge_range edge_range_;
	typedef typename base_t::edge_equiv_type edge_equiv_;
public:	
    
    or_filter ( predicate_type predicate   
              , boost::shared_ptr<span_filter_t> true_filter
              , boost::shared_ptr<span_filter_t> false_filter
              , span_t const& target_span
              , GramT& gram
              , concrete_edge_factory<EdgeT,GramT>& ef
              , ChartT& chart );
        
    virtual void apply_rules( rule_range_ const& rr
                            , edge_range_ const& er1
                            , edge_range_ const& er2 );				
    virtual void finalize();
    virtual bool is_finalized() const;
    
    virtual edge_equivalence<EdgeT> const& top() const
    {
		if (over_false) return false_filter->top();
		else return true_filter->top();
	}	
	virtual bool empty() const;
    virtual void pop();

private:
    boost::shared_ptr<span_filter_t> true_filter;
    boost::shared_ptr<span_filter_t> false_filter;
    bool over_false;
    predicate_type predicate;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT, class BoolFunction>
class or_filter_factory
: public span_filter_factory<EdgeT,GramT,ChartT>
{
public:
    typedef span_filter_factory<EdgeT,GramT,ChartT> base_t;
    typedef span_filter_factory<EdgeT,GramT,ChartT> span_filter_factory_t;
    typedef BoolFunction predicate_type;
    
    virtual void print_settings(std::ostream &o) const 
    {
        o << "or_factory{" << "predicate=" << predicate.label() << "; "
          << " predicate-true=";
        true_factory->print_settings(o);
        o << "; "<< "predicate-false=";
        false_factory->print_settings(o);
        o << "}";
    }
    
    virtual bool adjust_for_retry(unsigned retry_i)
    {
        return any_change(
            true_factory->adjust_for_retry(retry_i),
            false_factory->adjust_for_retry(retry_i));
    }
    
    virtual edge_filter<EdgeT> unary_filter(span_t const& target_span)
    {
        return true_factory->unary_filter(target_span);
    }
    
    or_filter_factory ( boost::shared_ptr<span_filter_factory_t> const& true_f
                      , boost::shared_ptr<span_filter_factory_t> const& false_f
                      , span_t const& total_span 
                      , predicate_type predicate = predicate_type() );
    
    virtual 
    typename base_t::result_type create( span_t const& target_span
                                       , GramT& gram
                                       , concrete_edge_factory<EdgeT,GramT>& ef
                                       , ChartT& chart )
    {
        typedef span_filter_interface<EdgeT,GramT,ChartT> span_filter_t;
    
        boost::shared_ptr<span_filter_t> 
            true_filt(true_factory->create(target_span,gram,ef,chart));
        
        boost::shared_ptr<span_filter_t> 
            false_filt(false_factory->create(target_span,gram,ef,chart));
        
        return new or_filter<EdgeT,GramT,ChartT,BoolFunction>( predicate
                                                             , true_filt
                                                             , false_filt
                                                             , target_span
                                                             , gram
                                                             , ef
                                                             , chart );
    }

private:
    boost::shared_ptr<span_filter_factory_t> true_factory;
    boost::shared_ptr<span_filter_factory_t> false_factory;
    predicate_type predicate;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# include <sbmt/search/impl/or_filter.ipp>

# endif //     SBMT__SEARCH__OR_FILTER_HPP
