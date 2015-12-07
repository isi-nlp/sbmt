
namespace sbmt { namespace detail {

////////////////////////////////////////////////////////////////////////////////

template <class ST>
ordered_span<ST>::ordered_span( edge_equivalence_pool<edge_type>& epool
                              , edge_type const& e )
{
    cell_container.insert(cell_type(epool,e));
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
void ordered_span<ST>::insert_edge( edge_equivalence_pool<edge_type>& epool
                                  , edge_type const& e )
{
    //assert(e.span() == span());
    typename cell_container_t::template nth_index<1>::type::iterator
                pos = cell_container.template get<1>().find(e.root());
    if (pos == cell_container.template get<1>().end()) {
        cell_container.insert(cell_type(epool,e));
    } else {
        cell_container.template get<1>().modify(pos, inserting_edge(epool,e));
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
typename ordered_span<ST>::edge_range
ordered_span<ST>::edges(indexed_token t) const
{
    typename cell_container_t::template nth_index<1>::type::iterator
                pos = cell_container.template get<1>().find(t);
    if (pos == cell_container.template get<1>().end())
        return edge_range();
    return pos->edges();
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
std::pair<typename ordered_span<ST>::edge_iterator,bool>
ordered_span<ST>::find(edge_type const& e) const
{
    typename cell_container_t::template nth_index<1>::type::iterator
                pos = cell_container.template get<1>().find(e.root());
    if (pos == cell_container.template get<1>().end())
        return std::make_pair(edge_iterator(),false);
    return pos->find(e);

}

////////////////////////////////////////////////////////////////////////////////

//template <class ST>
//span_t ordered_span<ST>::span() const
//{
//    return cell_container.begin()->span();
//}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
typename ordered_span<ST>::range ordered_span<ST>::cells() const
{
    return range(cell_container.begin(), cell_container.end());
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::detail
