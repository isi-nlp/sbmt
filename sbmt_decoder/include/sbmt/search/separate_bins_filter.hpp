#ifndef   SBMT_SEARCH_SEPARATE_BINS_FILTER_HPP
#define   SBMT_SEARCH_SEPARATE_BINS_FILTER_HPP

# include <sbmt/search/or_filter.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

struct is_tag_rule {
    typedef bool result_type;
    
    template <class Gram>
    bool operator()(Gram const& gram, typename Gram::rule_type const& r) const
    {
        return not (gram.rule_lhs(r).type() == virtual_tag_token);
    }
    std::string label() const { return "is_tag_rule"; }
};

////////////////////////////////////////////////////////////////////////////////
///
///  uses a separate span_filter for virtual and non-virtual edges.
///  the type of span_filters used is arbitrary and independent.
///
////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class separate_bins_factory
: public or_filter_factory<EdgeT,GramT,ChartT,is_tag_rule>
{
    typedef or_filter_factory<EdgeT,GramT,ChartT,is_tag_rule> base_t;
    typedef span_filter_factory<EdgeT,GramT,ChartT> span_filter_factory_t;
public:
    separate_bins_factory( boost::shared_ptr<span_filter_factory_t> tag_f
                         , boost::shared_ptr<span_filter_factory_t> virt_f
                         , span_t const& total_span )
     : base_t(tag_f,virt_f,total_span) {}
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT_SEARCH_SEPARATE_BINS_FILTER_HPP
