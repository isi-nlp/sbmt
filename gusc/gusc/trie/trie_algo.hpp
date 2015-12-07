# if ! defined(GUSC__TRIE__TRIE_ALGO_HPP)
# define       GUSC__TRIE__TRIE_ALGO_HPP

# include <gusc/functional.hpp>
# include <utility>
# include <boost/tuple/tuple.hpp>
# include <boost/graph/graph_traits.hpp>
# include <boost/graph/properties.hpp>
# include <boost/range.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
///  given a state s in trie, and a transition key, return the new state if any.
///
////////////////////////////////////////////////////////////////////////////////
template <class Trie, class Eq>
boost::iterator_range<typename Trie::iterator>
trie_transition( Trie const& trie
               , typename Trie::state s
               , typename Trie::key_type const& k
               , Eq eq );
               
template <class Trie, class OutIterator>
OutIterator trie_path(Trie const& trie, typename Trie::state s, OutIterator out)
{
    if (trie.start() != s) {
        out = trie_path(trie,trie.parent(s),out);
        *out = trie.key(s);
        ++out;
    }
    return out;
}

////////////////////////////////////////////////////////////////////////////////
///
///  find the state that corresponds to moving through the trie along the 
///  sequence [itr,end), beginning at trie.start(), if any
///
////////////////////////////////////////////////////////////////////////////////
//template <class Trie, class Iterator>
//std::pair<bool,typename Trie::state>
//trie_find(Trie const& trie, Iterator itr, Iterator end);

////////////////////////////////////////////////////////////////////////////////

template <class Trie1, class Trie2>
bool equal_trie(Trie1 const& t1, Trie2 const& t2);

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Graph, class PropertyMap, class OutIterator, class Equal>
void trie_search( Trie const& tr
                , Graph const& g
                , PropertyMap const& pmap
                , typename Trie::state root
                , typename boost::graph_traits<Graph>::vertex_descriptor start
                , OutIterator out
                , Equal eq );

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Graph, class PropertyMap, class OutIterator, class Equal>
void trie_search( Trie const& tr
                , Graph const& grph
                , PropertyMap const& pmap
                , typename boost::graph_traits<Graph>::vertex_descriptor start
                , OutIterator out
                , Equal eq );

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Graph, class OutIterator, class Equal>
void trie_search( Trie const& tr
                , Graph const& grph
                , typename boost::graph_traits<Graph>::vertex_descriptor start
                , OutIterator out
                , Equal eq )
{
    trie_search(tr,grph,get(boost::edge_name_t(),grph),start,out,eq);
}

////////////////////////////////////////////////////////////////////////////////

struct less_construct {
    typedef less result_type;
    template <class X> 
    less operator()(X const& x) const { return less(); }
};

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Graph, class OutIterator>
void trie_search( Trie const& tr
                , Graph const& grph
                , typename boost::graph_traits<Graph>::vertex_descriptor start
                , OutIterator out )
{
    trie_search( tr
               , grph
               , get(boost::edge_name_t(),grph)
               , start
               , out
               , less_construct() 
               );
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# include "impl/trie_algo.ipp"

# endif //     GUSC__TRIE__TRIE_ALGO_HPP
