#ifndef   SBMT__SEARCH__QUASI_BIN_FILTER_HPP
#define   SBMT__SEARCH__QUASI_BIN_FILTER_HPP

# include <sbmt/search/or_filter.hpp>
# include <sbmt/search/span_filter.hpp>
# include <sbmt/search/filter_predicates.hpp>
# include <boost/shared_ptr.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

struct is_quasi_bin {
    typedef bool result_type;
    template <class Gram>
    bool operator()(Gram const& gram, typename Gram::rule_type const& r) const
    {
        if (gram.rule_lhs(r).type() == tag_token) return true;
        else {
            for (size_t x = 0; x != gram.rule_rhs_size(r); ++x) {
                if (gram.rule_rhs(r,x).type() == foreign_token) return false;
            }
        }
        return true;
    }
    std::string label() const { return "is_quasi_bin"; }
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class quasi_bin_factory
: public or_filter_factory<EdgeT,GramT,ChartT,is_quasi_bin>
{
    typedef exhaustive_span_factory<EdgeT,GramT,ChartT> 
            sub_bin_factory_t;
            
    typedef or_filter_factory<EdgeT,GramT,ChartT,is_quasi_bin> 
            base_t;
            
    typedef span_filter_factory<EdgeT,GramT,ChartT> 
            span_filter_factory_t;
public:
    quasi_bin_factory( boost::shared_ptr<span_filter_factory_t> filt
                     , span_t const& total_span )
     : base_t( filt
             , boost::shared_ptr<span_filter_factory_t>(
                  new sub_bin_factory_t(make_predicate_edge_filter<EdgeT>(pass_thru_predicate()),total_span)
                )
             , total_span
             ) {}
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT__SEARCH__QUASI_BIN_FILTER_HPP
