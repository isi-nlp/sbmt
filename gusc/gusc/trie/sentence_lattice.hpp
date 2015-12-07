# if ! defined(GUSC__SENTENCE_LATTICE_HPP)
# define       GUSC__SENTENCE_LATTICE_HPP

# include <boost/graph/graph_traits.hpp>
# include <boost/tuple/tuple.hpp>
# include <iterator>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Graph, class EdgePropertyMap, class Iterator, class WildCard>
boost::tuple<
    typename boost::graph_traits<Graph>::vertex_descriptor
  , typename boost::graph_traits<Graph>::vertex_descriptor
>
skip_lattice_from_sentence( Graph& g
                          , EdgePropertyMap pmap
                          , Iterator itr
                          , Iterator end
                          , WildCard skip
                          );

////////////////////////////////////////////////////////////////////////////////
///
///  skip_lattice_from_sentence(g,get(edge_name_t(),g),itr,end,skip)
///
////////////////////////////////////////////////////////////////////////////////
template <class Graph, class Iterator, class WildCard>
boost::tuple<
    typename boost::graph_traits<Graph>::vertex_descriptor
  , typename boost::graph_traits<Graph>::vertex_descriptor
>
skip_lattice_from_sentence( Graph& g
                          , Iterator itr
                          , Iterator end
                          , WildCard skip
                          );

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# include "impl/sentence_lattice.ipp"

# endif //     GUSC__SENTENCE_LATTICE_HPP

