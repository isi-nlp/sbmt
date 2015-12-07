
namespace sbmt { namespace detail {

////////////////////////////////////////////////////////////////////////////////

template <class ET>
ordered_cell<ET>::ordered_cell( edge_equivalence_pool<ET>& epool
                              , edge_type const& e )
{
    edge_container.insert(epool.create(e));
}

template <class ET>
ordered_cell<ET>::ordered_cell( edge_equivalence<ET> const& eq )
{
    edge_container.insert(eq);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
void 
ordered_cell<ET>::insert_edge( edge_equivalence_pool<ET>& epool
                             , edge_type const& e )
{
    assert(e.span() == span() and e.root() == root());
    typename edge_container_t::template nth_index<1>::type::iterator
                pos = edge_container.template get<1>().find(e);
    if (pos == edge_container.template get<1>().end()) {
        edge_container.insert(epool.create(e));
    } else {
        edge_container.template get<1>().modify(pos,inserting_edge(e));
    }
}

template <class ET>
void 
ordered_cell<ET>::insert_edge( edge_equiv_type const& eq )
{
    assert( eq.representative().span() == span() and 
            eq.representative().root() == root() );
    typename edge_container_t::template nth_index<1>::type::iterator
                pos = edge_container.template get<1>().find(eq.representative());
    if (pos == edge_container.template get<1>().end()) {
        edge_container.insert(eq);
    } else {
        edge_container.template get<1>().modify(pos,merging_edges(eq));
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
span_t ordered_cell<ET>::span() const
{
    return edge_container.begin()->span();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
indexed_token ordered_cell<ET>::root() const
{
    return edge_container.begin()->representative().root();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
score_t ordered_cell<ET>::best_score() const
{
    return edge_container.begin()->representative().score();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
typename ordered_cell<ET>::range ordered_cell<ET>::edges() const
{
    return range(edge_container.begin(), edge_container.end());
}

template <class ET>
std::pair<typename ordered_cell<ET>::iterator,bool>
ordered_cell<ET>::find(edge_type const& e) const
{
    typename edge_container_t::template nth_index<1>::type const& 
                   ec = edge_container.template get<1>();
    iterator itr = edge_container.template project<0>(ec.find(e));
    return std::make_pair(itr, itr != edge_container.end());
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::detail
