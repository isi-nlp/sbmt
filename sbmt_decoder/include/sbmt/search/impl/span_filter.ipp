namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
template <class SpanFilterFunc>
exhaustive_span_filter<ET,GT,CT>::
exhaustive_span_filter( SpanFilterFunc sf
                      , span_t const& target_span
                      , GT& gram
                      , concrete_edge_factory<ET,GT>& ef
                      , CT& chart )
: base_t(target_span,gram,ef,chart)
, filter(sf) {}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void 
exhaustive_span_filter<ET,GT,CT>::apply_rules( 
                          rule_range_ const& rr
                        , edge_range_ const& er1
                        , edge_range_ const& er2
                        )
{
    typename base_t::rule_iterator ri, re;
    boost::tie(ri,re) = rr;
    
    for (;ri != re; ++ri) {
        apply_rule(*ri,er1,er2);
    }
}

template <class ET, class GT, class CT>
void 
exhaustive_span_filter<ET,GT,CT>::apply_rule( 
                                rule_type_ const& r
                              , edge_range_ const& er1
                              , edge_range_ const& er2
                              )
{
    assert(base_t::gram.rule_rhs_size(r) == 2);

    typename chart_traits<CT>::edge_iterator e1itr = er1.begin();
    typename chart_traits<CT>::edge_iterator e1end = er1.end();
    
    for (; e1itr != e1end; ++e1itr) {
        typename chart_traits<CT>::edge_iterator e2itr = er2.begin();
        typename chart_traits<CT>::edge_iterator e2end = er2.end();
        for (; e2itr != e2end; ++e2itr) {
            gusc::any_generator<ET> g = base_t::ef.create_edge(
                                               base_t::gram
                                             , r
                                             , *e1itr
                                             , *e2itr
                                           );
            while (g) {
                filter.insert(base_t::epool, g());
            }
            
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void exhaustive_span_filter<ET,GT,CT>::finalize()
{
    filter.finalize();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool exhaustive_span_filter<ET,GT,CT>::is_finalized() const
{
    return filter.is_finalized();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool exhaustive_span_filter<ET,GT,CT>::empty() const
{
    return filter.empty();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void exhaustive_span_filter<ET,GT,CT>::pop()
{
    return filter.pop();
}
////////////////////////////////////////////////////////////////////////////////


} // namespace sbmt
