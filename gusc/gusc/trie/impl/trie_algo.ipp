# include <set>
# include <algorithm>
# include <boost/foreach.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Trie,class Less>
struct trie_less {
    Trie const* trie;
    Less op;
    trie_less(Trie const& trie, Less const& op) : trie(&trie), op(op) {}
    
    bool operator()( typename Trie::state const& p
                   , typename Trie::key_type const& v ) const 
    { 
        return op(trie->key(p),v);
    }
    
    bool operator()( typename Trie::key_type const& v
                   , typename Trie::state const& p ) const 
    { 
        return op(v,trie->key(p));
    }
};

template <class V, class L>
trie_less<V,L> make_trie_less(V const& v, L const& l) 
{ 
    return trie_less<V,L>(v,l); 
}

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Equal>
boost::iterator_range<typename Trie::iterator>
trie_transition( Trie const& trie
               , typename Trie::state s
               , typename Trie::key_type const& k
               , Equal eq )
{
    typedef typename Trie::iterator pos_t;
    
    pos_t beg, end;
    boost::tie(beg,end) = trie.transitions(s);

    pos_t low = std::lower_bound(beg, end, k, make_trie_less(trie, eq(k)));
    pos_t high = std::upper_bound(low, end, k, make_trie_less(trie,eq(k)));
    
    return boost::make_iterator_range(low,high);
}

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Iterator>
struct trie_find_result {
    bool found;
    Iterator pos;
    typename Trie::state state;
};

template <class Trie, class Iterator>
trie_find_result<Trie,Iterator>
trie_find(Trie const& trie, typename Trie::state s, Iterator itr, Iterator end)
{
    bool found = true;
    std::pair<bool,typename Trie::state> p;
    for (; itr != end; ++itr) {
        p = trie.transition(s,*itr);
        if (not p.first) {
            found = false;
            break;
        }
        s = p.second;
    }
    if (itr != end or trie.value(s) == trie.nonvalue()) found = false;
    trie_find_result<Trie,Iterator> t = {found,itr,s};
    return t;
}


////////////////////////////////////////////////////////////////////////////////

template <class Trie1, class Trie2>
bool equal_trie( Trie1 const& t1
               , typename Trie1::state const& s1
               , Trie2 const& t2
               , typename Trie2::state const& s2 )
{
    //assert(t1.value(s1) == t2.value(s2));
    if (t1.value(s1) != t2.value(s2)) return false;
    
    typename Trie1::iterator i1,e1;
    boost::tie(i1,e1) = t1.transitions(s1);
    
    typename Trie2::iterator i2,e2;
    boost::tie(i2,e2) = t2.transitions(s2);
    
    //assert(std::distance(i1,e1) == std::distance(i2,e2));
    if (std::distance(i1,e1) != std::distance(i2,e2)) return false;
    for (; i1 != e1; ++i1, ++i2) {
        //assert(t1.key(*i1) == t2.key(*i2));
        if (t1.key(*i1) != t2.key(*i2)) return false;
        if (not equal_trie(t1,*i1,t2,*i2)) return false;
    }
    if (i2 != e2) return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

template <class Trie1, class Trie2>
bool equal_trie(Trie1 const& t1, Trie2 const& t2)
{
    return equal_trie(t1,t1.start(),t2,t2.start());
}

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Graph> 
struct trie_search_traits {
    typedef typename Trie::state in_state;
    typedef typename boost::graph_traits<Graph>::vertex_descriptor out_state;
    typedef boost::tuple<in_state, out_state> state;
    typedef std::set<state> state_set;
    typedef boost::tuple<typename Trie::value_type, out_state> value_type;
};

template <class Trie, class Graph, class PropertyMap, class OutIterator, class Equal>
void trie_search_impl( Trie const& tr
                     , Graph const& g
                     , PropertyMap const& pmap
                     , typename trie_search_traits<Trie,Graph>::state_set& equivset
                     , typename trie_search_traits<Trie,Graph>::state s
                     , OutIterator out
                     , Equal eq )
{
    typename trie_search_traits<Trie,Graph>::in_state in_s;
    typename trie_search_traits<Trie,Graph>::out_state out_s;
    boost::tie(in_s,out_s) = s;
    
    typename trie_search_traits<Trie,Graph>::value_type 
        v(tr.value(in_s),out_s);
    if (boost::get<0>(v) != tr.nonvalue()) {
        *out = v;
        ++out;
    }
    
    BOOST_FOREACH( typename boost::graph_traits<Graph>::edge_descriptor edge
                 , out_edges(out_s,g) ) {
        typename boost::property_traits<PropertyMap>::value_type tok = pmap[edge];//get(pmap,edge);
        BOOST_FOREACH( typename Trie::state to
                     , trie_transition(tr,in_s,tok,eq) ) {
            typename trie_search_traits<Trie,Graph>::state 
                ss = boost::make_tuple(to,target(edge,g));
            if(equivset.insert(ss).second) {
                trie_search_impl(tr,g,pmap,equivset,ss,out,eq);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Graph, class PropertyMap, class OutIterator, class Equal>
void trie_search( Trie const& tr
                , Graph const& g
                , PropertyMap const& pmap
                , typename Trie::state root
                , typename boost::graph_traits<Graph>::vertex_descriptor start
                , OutIterator out
                , Equal eq )
{
    typename trie_search_traits<Trie,Graph>::state_set equivset;
    trie_search_impl(tr,g,pmap,equivset,boost::make_tuple(root,start),out,eq);
}

////////////////////////////////////////////////////////////////////////////////

template <class Trie, class Graph, class PropertyMap, class OutIterator, class Equal>
void trie_search( Trie const& tr
                , Graph const& g
                , PropertyMap const& pmap
                , typename boost::graph_traits<Graph>::vertex_descriptor start
                , OutIterator out
                , Equal eq )
{
    trie_search(tr,g,pmap,tr.start(),start,out,eq);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc
