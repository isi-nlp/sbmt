# include <list>
# include <boost/graph/properties.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Graph, class EdgePropertyMap, class Iterator, class WildCards>
boost::tuple<
    typename boost::graph_traits<Graph>::vertex_descriptor
  , typename boost::graph_traits<Graph>::vertex_descriptor
>
skip_lattice_from_sentence( Graph& g
                          , EdgePropertyMap pmap
                          , Iterator itr
                          , Iterator end
                          , WildCards skip
                          )
{
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef typename boost::graph_traits<Graph>::edge_descriptor edge_t;
    std::list<vertex_t> connect_me;
    
    size_t id = 0;
    vertex_t start = add_vertex(id,g);
    vertex_t curr = start;
    ++id;
    for (; itr != end; ++itr) {
        vertex_t next = add_vertex(id,g);
        size_t s = id;
        ++id;
        edge_t e = add_edge(curr,next,g).first;
        put(pmap,e,*itr);
        connect_me.push_back(curr);
        typename std::list<vertex_t>::iterator li = connect_me.begin(), 
                                               le = connect_me.end();
        
        
        for (; li != le; ++li,--s) {
            e = add_edge(*li,next,g).first;
            put(pmap,e,skip[s]);
        }
        assert(s == 0);
        curr = next;
    }
    
    vertex_t finish = curr;
    return boost::make_tuple(start,finish);
}

////////////////////////////////////////////////////////////////////////////////

template <class Graph, class Iterator, class WildCards>
boost::tuple<
    typename boost::graph_traits<Graph>::vertex_descriptor
  , typename boost::graph_traits<Graph>::vertex_descriptor
>
skip_lattice_from_sentence( Graph& g
                          , Iterator itr
                          , Iterator end
                          , WildCards skip
                          )
{
    return skip_lattice_from_sentence( g
                                     , get(boost::edge_name_t(),g)
                                     , itr
                                     , end
                                     , skip );
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc
