# if ! defined(GUSC__TRIE__TRAVERSE_TRIE_HPP)
# define       GUSC__TRIE__TRAVERSE_TRIE_HPP

# include <boost/tuple/tuple.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Visitor>
void traverse_trie(Trie& trie, typename Trie::state s, Visitor visit)
{
    visit.at_state(trie,s);
    visit.begin_children(trie,s);
    typename Trie::iterator itr, end;
    boost::tie(itr,end) = trie.transitions(s);
    for (; itr != end; ++itr) { traverse_trie(trie,*itr,visit); }
    visit.end_children(trie,s);
}

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Visitor>
void traverse_trie(Trie& trie, Visitor visit)
{
    traverse_trie(trie,trie.start(),visit);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__TRIE__TRAVERSE_TRIE_HPP
