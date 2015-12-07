# include <boost/bind.hpp>
# include <boost/iterator/filter_iterator.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT, class BF>
or_filter<ET,GT,CT,BF>::
or_filter( predicate_type predicate
         , boost::shared_ptr<span_filter_t> true_filter
         , boost::shared_ptr<span_filter_t> false_filter
         , span_t const& target_span
         , GT& gram
         , concrete_edge_factory<ET,GT>& ef
         , CT& chart )
: base_t(target_span,gram,ef,chart)
, true_filter(true_filter)
, false_filter(false_filter)
, over_false(false) 
, predicate(predicate) {}

template <class E, class G, class C, class B>
void 
or_filter<E,G,C,B>::apply_rules( rule_range_ const& rr
       						   , edge_range_ const& er1
						       , edge_range_ const& er2 )
{
	using namespace boost;
	typename base_t::rule_iterator ritr, rend;
	tie(ritr,rend) = rr;
	if (ritr == rend) return;

	// make a rule range that filters on the rules that satisfy
	// predicate(gram,*ritr) == true and feed it to
	// true_filter->apply_rules
	true_filter->apply_rules(
		make_tuple(
			make_filter_iterator(
				bind(predicate,cref(base_t::gram),_1), ritr, rend
			)
		  , make_filter_iterator(
				bind(predicate,cref(base_t::gram),_1), rend, rend
			)
		)
	  , er1
	  , er2
	);
	
	// make a rule range that filters on the rules that satisfy
	// predicate(gram,*ritr) == false and feed it to
	// false_filter->apply_rules
	false_filter->apply_rules(
		make_tuple(
			make_filter_iterator(
				! bind(predicate,cref(base_t::gram),_1), ritr, rend
			)
		  , make_filter_iterator(
				! bind(predicate,cref(base_t::gram),_1), rend, rend
			)
		)
	  , er1
	  , er2
	);
}	

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT, class BF>                            
void or_filter<ET,GT,CT,BF>::finalize()
{
    false_filter->finalize();
    true_filter->finalize();
    over_false = not false_filter->empty();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT, class BF>
bool or_filter<ET,GT,CT,BF>::is_finalized() const
{
    return false_filter->is_finalized() and true_filter->is_finalized();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT, class BF>
bool or_filter<ET,GT,CT,BF>::empty() const
{
    return false_filter->empty() and true_filter->empty();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT, class BF>
void or_filter<ET,GT,CT,BF>::pop()
{
    if (over_false) {
        false_filter->pop();
        if (false_filter->empty()) over_false = false;
    } else {
        true_filter->pop();
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT, class BF>
or_filter_factory<ET,GT,CT,BF>::
or_filter_factory( boost::shared_ptr<span_filter_factory_t> const& true_f
                 , boost::shared_ptr<span_filter_factory_t> const& false_f
                 , span_t const& total_span
                 , predicate_type predicate )
: base_t(total_span)
, true_factory(true_f)
, false_factory(false_f)
, predicate(predicate)
{}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
