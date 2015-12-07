namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
template <class FilterFunc>
cell_bin_filter<ET,GT,CT>::cell_bin_filter( boost::shared_ptr<base_t> span_filt
                                          , FilterFunc filter_func
                                          , span_t const& target_span
                                          , GT& gram
                                          , concrete_edge_factory<ET,GT>& ecs
                                          , CT& chart )
: base_t(target_span, gram, ecs, chart)
, span_filt(span_filt)
, filter_func(filter_func)
, current(cell_filters.end())
{}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void cell_bin_filter<ET,GT,CT>::finalize()
{
    span_filt->finalize();
    while (not span_filt->empty()) {
        const typename base_t::edge_equiv_type& eq = span_filt->top();
        typename filter_table_t::iterator pos = 
                        cell_filters.find(eq.representative().root());
        if (pos == cell_filters.end()) {
            pos = insert_into_map( cell_filters
                                 , eq.representative().root()
                                 , edge_filter_t(filter_func) ).first;
        }
        //const_cast<edge_filter_t&>(pos->second).insert(eq);
        pos->second.insert(eq);
        span_filt->pop();
    }
    current = cell_filters.begin();
    end     = cell_filters.end();
    while (current != end and current->second.empty()) {
        ++current;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool cell_bin_filter<ET,GT,CT>::is_finalized() const
{
    return span_filt->is_finalized();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool cell_bin_filter<ET,GT,CT>::empty() const
{
    return current == end;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void cell_bin_filter<ET,GT,CT>::pop()
{
    const_cast<edge_filter_t&>(current->second).pop();
    while (current != end and current->second.empty()) {
        ++current;
    }
}
////////////////////////////////////////////////////////////////////////////////



} // namespace sbmt

