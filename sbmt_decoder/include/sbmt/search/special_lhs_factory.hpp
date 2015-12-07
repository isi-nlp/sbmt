# if ! defined(SBMT__SEARCH__SPECIAL_LHS_FACTORY_HPP)
# define       SBMT__SEARCH__SPECIAL_LHS_FACTORY_HPP

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

struct is_special_lhs {
    typedef bool result_type;
    
    template <class Gram>
    bool operator()(Gram const& gram, typename Gram::rule_type const& r) const
    {
        return (gram.rule_lhs(r) == tok);
    }
    std::string label() const { return "is_special_rhs"; }
    
    is_special_lhs(indexed_token tok) : tok(tok) {}
    
private:
    indexed_token tok;
};

////////////////////////////////////////////////////////////////////////////////
///
///  uses a separate span_filter for virtual and non-virtual edges.
///  the type of span_filters used is arbitrary and independent.
///
////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class special_lhs_factory
: public or_filter_factory<EdgeT,GramT,ChartT,is_special_lhs>
{
    typedef or_filter_factory<EdgeT,GramT,ChartT,is_special_lhs> base_t;
    typedef span_filter_factory<EdgeT,GramT,ChartT> span_filter_factory_t;
public:
    special_lhs_factory( boost::shared_ptr<span_filter_factory_t> is_special
                       , boost::shared_ptr<span_filter_factory_t> is_not_special
                       , indexed_token tok
                       , span_t const& total_span )
     : base_t(is_special,is_not_special,total_span,is_special_lhs(tok)) {}
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__SEARCH__SPECIAL_LHS_FACTORY_HPP
