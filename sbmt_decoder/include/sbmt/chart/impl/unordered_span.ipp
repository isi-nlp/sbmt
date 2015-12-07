
namespace sbmt { namespace detail {

////////////////////////////////////////////////////////////////////////////////

template <class CT>
unordered_span<CT>::unordered_span( edge_equivalence_pool<edge_type>& epool
                                  , edge_type const& e )
{
    cell_container.insert(cell_type_ptr(new cell_type(epool,e)));
}

template <class CT>
unordered_span<CT>::unordered_span(edge_equiv_type const& eq )
{
    cell_container.insert(cell_type_ptr(new cell_type(eq)));
}

////////////////////////////////////////////////////////////////////////////////

template <class CT>
void unordered_span<CT>::insert_edge( edge_equivalence_pool<edge_type>& epool
                                    , edge_type const& e )
{
    //assert(e.span() == span());
    typename cell_container_t::iterator pos = cell_container.find(e.root());
    if (pos == cell_container.end()) {
        cell_container.insert(cell_type_ptr(new cell_type(epool,e)));
    } else {
        cell_container.modify(pos, inserting_edge(epool,e));
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class CT>
void unordered_span<CT>::insert_edge(edge_equiv_type const& eq)
{
   // assert(eq.representative().span() == span());
    typename cell_container_t::iterator pos = 
                        cell_container.find(eq.representative().root());
    if (pos == cell_container.end()) {
        cell_container.insert(cell_type_ptr(new cell_type(eq)));
    } else {
        cell_container.modify(pos, merging_edges(eq));
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class CT>
typename unordered_span<CT>::range unordered_span<CT>::cells() const
{
    return range( iterator(cell_container.begin())
                , iterator(cell_container.end()) );
}

template <class CT>
std::pair<typename unordered_span<CT>::iterator,bool>
unordered_span<CT>::cell(indexed_token t) const
{
    typename cell_container_t::iterator pos = cell_container.find(t);
    return std::make_pair(iterator(pos), pos != cell_container.end());
}

////////////////////////////////////////////////////////////////////////////////

template <class CT>
typename unordered_span<CT>::edge_range
unordered_span<CT>::edges(indexed_token t) const
{
    typename cell_container_t::iterator pos = cell_container.find(t);
    if (pos == cell_container.end()) {
        return edge_range();
    } 
    return (*pos)->edges();
}

////////////////////////////////////////////////////////////////////////////////

template <class ST>
std::pair<typename unordered_span<ST>::edge_iterator,bool>
unordered_span<ST>::find(edge_type const& e) const
{
    typename cell_container_t::iterator
                pos = cell_container.find(e.root());
    if (pos == cell_container.end())
        return std::make_pair(edge_iterator(),false);
    return (*pos)->find(e);
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::detail
 
