
namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class ET>
edge_queue<ET>::edge_queue()
: high_score(0.0) {}

////////////////////////////////////////////////////////////////////////////////

template <class ET> 
typename edge_queue<ET>::iterator
edge_queue<ET>::insert(edge_equivalence_pool<ET>& epool, edge_type const& e)

{
    typename priority_table_t::iterator pos = ptable.find(e);
    if (pos == ptable.end()) {
        pos = ptable.insert(epool.create(e)).first;
    }
    else {
        edge_inserter<edge_type> f(e);
        ptable.modify(pos,f);
        pos = ptable.find(e);
    }
    if (e.score() > high_score) high_score = e.score();
    assert(pos != ptable.end());
    return pos;
}

////////////////////////////////////////////////////////////////////////////////
         
template <class ET>
typename edge_queue<ET>::iterator
edge_queue<ET>::insert(edge_equiv_type const& eq)
{
    typename priority_table_t::iterator pos = ptable.find(eq.representative());
    if (pos == ptable.end()) pos = ptable.insert(eq).first;
    else {
        edge_equivalence_merge<edge_type> f(eq);
        ptable.modify(pos,f);
        pos = ptable.find(eq.representative());
    }
    if (eq.representative().score() > high_score) {
        high_score = eq.representative().score();
    }
    assert(pos != ptable.end());
    return pos;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
typename edge_queue<ET>::iterator
edge_queue<ET>::begin() { return ptable.begin(); }

////////////////////////////////////////////////////////////////////////////////

template <class ET>
typename edge_queue<ET>::iterator
edge_queue<ET>::end() { return ptable.end(); }

////////////////////////////////////////////////////////////////////////////////

template <class ET>
bool
edge_queue<ET>::erase(edge_type const& e)
{
    iterator pos = ptable.find(e);
    if (pos == ptable.end()) return false;
    score_t pos_score = pos->representative().score();
    ptable.erase(pos);
    if (pos_score >= high_score) {
        score_t max_score = 0.0;
        iterator itr = ptable.begin(), end = ptable.end();
        for (; itr != end; ++itr) {
            max_score = std::max(max_score,itr->representative().score());
        }
        high_score = max_score;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
typename edge_queue<ET>::edge_equiv_type const& edge_queue<ET>::top() const
{
    return ptable.top();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
void edge_queue<ET>::pop()
{
    ptable.pop();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
score_t edge_queue<ET>::lowest_score() const
{
    if (!empty()) return ptable.top().representative().score();
    else return score_t(0.0);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
score_t edge_queue<ET>::highest_score() const
{
    if (!empty()) return high_score;
    else return score_t(0.0);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
std::size_t edge_queue<ET>::size() const
{
    return ptable.size();
}

////////////////////////////////////////////////////////////////////////////////

template <class ET>
bool edge_queue<ET>::empty() const
{
    return ptable.empty();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
