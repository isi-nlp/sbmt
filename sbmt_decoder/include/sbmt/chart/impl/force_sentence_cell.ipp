
namespace sbmt { namespace detail {

////////////////////////////////////////////////////////////////////////////////

template <class ET>
force_sentence_cell<ET>::force_sentence_cell( edge_equivalence_pool<ET>& epool
                                            , edge_type const& e )
: best_scr(0.0)
{
    edge_container.insert(epool.create(e));
    best_scr = e.score();
}

template <class ET>
force_sentence_cell<ET>::force_sentence_cell(edge_equiv_type const& eq )
: best_scr(0.0)
{
    edge_container.insert(eq);
    best_scr = eq.representative().score();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
void 
force_sentence_cell<ET>::insert_edge( edge_equivalence_pool<ET>& epool
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
    best_scr = std::max(best_scr,e.score());
}

template <class ET>
void force_sentence_cell<ET>::insert_edge(edge_equiv_type const& eq )
{
    assert(eq.span() == span() and eq.representative().root() == root());
    typename edge_container_t::template nth_index<1>::type::iterator 
                 pos = edge_container.template get<1>().find(eq.representative());
    if (pos == edge_container.template get<1>().end()) {
        edge_container.insert(eq);
    } else {
        edge_container.template get<1>().modify(pos, merging_edges(eq));
    }
    best_scr = std::max(best_scr,eq.representative().score());
}


////////////////////////////////////////////////////////////////////////////////

template <class ET>
span_t force_sentence_cell<ET>::span() const
{
    return edge_container.begin()->span();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
indexed_token force_sentence_cell<ET>::root() const
{
    return edge_container.begin()->representative().root();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
score_t force_sentence_cell<ET>::best_score() const
{
    return best_scr;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
typename force_sentence_cell<ET>::range force_sentence_cell<ET>::edges() const
{
    return range(edge_container.begin(), edge_container.end());
}

template <class ET>
typename force_sentence_cell<ET>::range 
force_sentence_cell<ET>::edges(span_index_t left) const
{
    std::pair<iterator,iterator> p = edge_container.equal_range(left);
    return range(p.first,p.second);
}

template <class ET>
std::pair<typename force_sentence_cell<ET>::iterator,bool>
force_sentence_cell<ET>::find(edge_type const& e) const
{
    typename edge_container_t::template nth_index<1>::type const& 
                   ec = edge_container.template get<1>();
    iterator itr = edge_container.template project<0>(ec.find(e));
    return std::make_pair(itr, itr != edge_container.end());
}

////////////////////////////////////////////////////////////////////////////////

} } // namespace sbmt::detail
