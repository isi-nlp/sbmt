
namespace sbmt { namespace detail {

////////////////////////////////////////////////////////////////////////////////

template <class ET>
unordered_cell<ET>::unordered_cell( edge_equivalence_pool<ET>& epool
                                  , edge_type const& e )
: best_scr(0.0)
{
    edge_container.insert(epool.create(e));
    best_scr = e.score();
}

template <class ET>
unordered_cell<ET>::unordered_cell(edge_equiv_type const& eq )
: best_scr(0.0)
{
    edge_container.insert(eq);
    best_scr = eq.representative().score();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
void unordered_cell<ET>::insert_edge( edge_equivalence_pool<ET>& epool
                                    , edge_type const& e )
{
    assert(e.span() == span() and e.root() == root());
    typename edge_container_t::iterator pos = edge_container.find(e);
    if (pos == edge_container.end()) {
        edge_container.insert(epool.create(e));
    } else {
        edge_container.modify(pos,inserting_edge(e));
    }
    best_scr = std::max(best_scr,e.score());
}

template <class ET>
void unordered_cell<ET>::insert_edge(edge_equiv_type const& eq )
{
    assert(eq.span() == span() and eq.representative().root() == root());
    typename edge_container_t::iterator pos = 
                                edge_container.find(eq.representative());
    if (pos == edge_container.end()) {
        edge_container.insert(eq);
    } else {
        edge_container.modify(pos, merging_edges(eq));
    }
    best_scr = std::max(best_scr,eq.representative().score());
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
span_t unordered_cell<ET>::span() const
{
    return edge_container.begin()->span();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
indexed_token unordered_cell<ET>::root() const
{
    return edge_container.begin()->representative().root();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
score_t unordered_cell<ET>::best_score() const
{
    return best_scr;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
typename unordered_cell<ET>::range unordered_cell<ET>::edges() const
{
    return range(edge_container.begin(), edge_container.end());
}

template <class ET>
std::pair<typename unordered_cell<ET>::iterator,bool>
unordered_cell<ET>::find(edge_type const& e) const
{
    iterator pos = edge_container.find(e);
    return std::make_pair(pos, pos != edge_container.end());
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::detail
