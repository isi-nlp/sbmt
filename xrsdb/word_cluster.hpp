# if ! defined(XRSDB__WORD_CLUSTER_HPP)
# define       XRSDB__WORD_CLUSTER_HPP

# include <sbmt/grammar/syntax_rule.hpp>
# include <sbmt/span.hpp>
# include <sbmt/search/lattice_reader.hpp>
# include <sbmt/hash/hash_map.hpp>

# include <gusc/trie/basic_trie.hpp>
# include <gusc/trie/fixed_trie.hpp>
# include <gusc/trie/trie_algo.hpp>
# include <gusc/iterator/reverse.hpp>

# include <boost/cstdint.hpp>
# include <boost/noncopyable.hpp>
# include <boost/serialization/binary_object.hpp>
# include <boost/operators.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/graph/graph_traits.hpp>
# include <boost/graph/properties.hpp>
# include <boost/graph/reverse_graph.hpp>
# include <boost/functional/hash.hpp>
# include <boost/range.hpp>

# include <set>
# include <vector>

# include <syntax_rule_util.hpp>
# include <filesystem.hpp>

namespace xrsdb {

////////////////////////////////////////////////////////////////////////////////

/// double-trie structure for building up a final word_cluster
class word_cluster_construct : boost::noncopyable {
public:
	typedef boost::uint32_t uint_type;
    word_cluster_construct(sbmt::indexed_token const& wd);
    template <class Range>
    void insert(Range const& key, std::string const& value);
    sbmt::indexed_token root_word() const { return wd; }
    void swap_self(word_cluster_construct& other);
private:
    typedef gusc::basic_trie<sbmt::indexed_token, uint_type> trie_t;

    sbmt::indexed_token wd;
    trie_t fwd;
    trie_t bkwd;
    uint_type fwd_id;
    uint_type bkwd_id;
    typedef stlext::hash_map<
              std::pair<uint_type, uint_type>
            , std::vector< std::string >
            , boost::hash< std::pair<uint_type,uint_type> >
            > syntax_rules_map_t;
    syntax_rules_map_t syntax_rules_map;

    friend class word_cluster;
};

////////////////////////////////////////////////////////////////////////////////


class cluster_search {
public:
    typedef sbmt::graph_t Graph;
    typedef boost::uint32_t uint_type;
    cluster_search();
    typedef std::vector< boost::tuple<sbmt::span_t,size_t> > backtrack_type;
    typedef boost::tuple<
              backtrack_type
            , word_trie::value_type 
            > single_result_type;
    typedef std::vector<single_result_type> result_type;
    
    cluster_search( header const& h
                  , word_trie const* trie
                  , Graph const* g
                  , boost::graph_traits<Graph>::vertex_descriptor b 
                  );
                  
    cluster_search( header const& h
                  , word_trie const* trie
                  , Graph const* g
                  , boost::graph_traits<Graph>::vertex_descriptor b
                  , boost::graph_traits<Graph>::edge_descriptor e 
                  );
    
    result_type operator()(boost::graph_traits<Graph>::vertex_descriptor f);
    
    void dump(std::ostream& os) const;
    word_trie const* search_trie() const { return trie; }
    
private:
    typedef word_trie::state input_state;
    typedef boost::graph_traits<Graph>::vertex_descriptor output_state;
    typedef boost::tuple<std::vector<boost::tuple<sbmt::span_t,size_t> >,input_state,output_state> state;
    typedef boost::tuple<sbmt::span_t,size_t,state> deriv;
    word_trie const* trie;
    Graph const* g;
    boost::graph_traits<Graph>::vertex_descriptor f;
    std::vector<state> states;
    header const* ph;
};



class word_cluster : boost::equality_comparable<word_cluster> {
public:
	typedef boost::uint32_t uint_type;
    word_cluster();

    size_t memory_size() const;

    word_cluster(word_cluster_construct const& t);
    sbmt::indexed_token root_word() const { return wd; }

    bool operator == (word_cluster const& wc) const;

    template <class Graph>
    struct value_type {
        char const* c_str;
        size_t      len;
        typename boost::graph_traits<Graph>::vertex_descriptor left;
        typename boost::graph_traits<Graph>::vertex_descriptor right;
    };
    
    void dump(std::ostream& os) const;

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  Graph is a graph consistent with boost interface.
    ///  PMap is an edge property map, mapping edges to indexed_tokens
    ///  OutIterator accepts integers (rule-ids)
    ///  pre-condition: pmap[e] == this->root_word()
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class Graph, class OutIterator>
    void search( Graph const& g
               , typename boost::graph_traits<Graph>::edge_descriptor e
               , OutIterator out ) const;
private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, unsigned int version);

    std::pair<uint_type const*, uint_type const*> rules(uint_type) const;

    typedef gusc::fixed_trie<
                sbmt::indexed_token
              , uint_type
            > trie_t;
    typedef std::pair<uint_type,uint_type> sig_key_t;
    struct syntax_key_value_t {
        sig_key_t key;
        uint_type  value_offset;
        uint_type  value_sz;
    };

    struct key_sort {

        bool operator()( syntax_key_value_t const& skv1
                       , syntax_key_value_t const& skv2 ) const
        {
            return skv1.key.first < skv2.key.first or
                  (skv1.key.first == skv2.key.first and
                   skv1.key.second < skv2.key.second
                  );
        }

    };

    struct left_key_sort {
        bool operator()( syntax_key_value_t const& skv
                       , uint_type const& lft ) const
        {
            return skv.key.first < lft;
        }

        bool operator()( uint_type const& lft
                       , syntax_key_value_t const& skv ) const
        {
            return lft < skv.key.first;
        }
    };

    uint_type syntax_rules_map_sz;
    uint_type syntax_rules_sz;
    boost::shared_array<syntax_key_value_t> syntax_rules_map;
    boost::shared_array<char> syntax_rules;

    sbmt::indexed_token wd;
    trie_t fwd;
    trie_t bkwd;
};

////////////////////////////////////////////////////////////////////////////////

template <class Graph>
std::ostream&
operator << (std::ostream& out, word_cluster::value_type<Graph> const& vt)
{
    out.write(vt.c_str,vt.len);
    return out;
}

template <class Graph>
size_t hash_value(word_cluster::value_type<Graph> const& vt)
{
    return size_t(vt.c_str);
}

template <class Graph>
bool operator == ( word_cluster::value_type<Graph> const& v1
                 , word_cluster::value_type<Graph> const& v2 )
{
    return size_t(v1.c_str) == size_t(v2.c_str);
}

////////////////////////////////////////////////////////////////////////////////

template <class Archive>
void word_cluster::serialize(Archive& ar, unsigned int version)
{
    using boost::serialization::make_binary_object;

    ar & wd;
    ar & fwd;
    ar & bkwd;
    ar & syntax_rules_map_sz;
    ar & syntax_rules_sz;

    if (Archive::is_loading::value) {
        syntax_rules_map.reset(new syntax_key_value_t[syntax_rules_map_sz]);
        syntax_rules.reset(new char[syntax_rules_sz]);
    }

    ar & make_binary_object( syntax_rules_map.get()
                           , syntax_rules_map_sz * sizeof(syntax_key_value_t) );
    ar & make_binary_object( syntax_rules.get()
                           , syntax_rules_sz * sizeof(char) );
}

////////////////////////////////////////////////////////////////////////////////

inline void swap(word_cluster_construct& o1, word_cluster_construct& o2)
{
    o1.swap_self(o2);
}

////////////////////////////////////////////////////////////////////////////////

template <class Range>
void word_cluster_construct::insert(Range const& rhs, std::string const& value)
{
    using namespace boost;

    std::pair<uint_type,uint_type> sigid;

    typename range_const_iterator<Range>::type beg = boost::begin(rhs),
                                               end = boost::end(rhs),
                                               itr;

    for (itr = beg; itr != end; ++itr) {
        if (*itr == wd) break;
    }
    if (itr == end) {
        throw std::logic_error("rhs does not contain rarest word");
    }
    typedef std::pair<bool,trie_t::state> pair_t;

    pair_t pf = fwd.insert( itr
                          , end
                          , fwd_id
                          );
    if (pf.first) {
        sigid.first = fwd_id;
        ++fwd_id;
    } else {
        sigid.first = fwd.value(pf.second);
    }

    pair_t pb = bkwd.insert( gusc::reverse(itr)
                           , gusc::reverse(beg)
                           , bkwd_id
                           );
    if (pb.first) {
        sigid.second = bkwd_id;
        ++bkwd_id;
    } else {
        sigid.second = bkwd.value(pb.second);
    }
    syntax_rules_map[sigid].push_back(value);
}

////////////////////////////////////////////////////////////////////////////////
// helper sorter
////////////////////////////////////////////////////////////////////////////////
struct lessfirst {
    typedef bool result_type;
    template <class T, class U>
    bool operator()(boost::tuple<T,U> const& t1, boost::tuple<T,U> const& t2) const
    {
        return boost::get<0>(t1) < boost::get<0>(t2);
    }
    template <class T, class U>
    bool operator()(boost::tuple<T,U> const& t1, T const& t2) const
    {
        return boost::get<0>(t1) < t2;
    }
    template <class T, class U>
    bool operator()(T const& t1, boost::tuple<T,U> const& t2) const
    {
        return t1 < boost::get<0>(t2);
    }

};

////////////////////////////////////////////////////////////////////////////////

template <class Graph, class OutIterator>
void
word_cluster::search( Graph const& g
                    , typename boost::graph_traits<Graph>::edge_descriptor ed
                    , OutIterator out ) const
{
    using namespace std;
    using namespace boost;
    typedef vector<
        tuple<
            uint_type
          , typename boost::graph_traits<Graph>::vertex_descriptor
        >
    > search_vec_t;
    search_vec_t fwd_vec, bkwd_vec;
    if(get(get(boost::edge_bundle,g),ed) != root_word()) {
        sbmt::indexed_token rt = get(get(boost::edge_bundle,g),ed);
        std::stringstream sstr;
        sstr << "rt=" << rt << " != " << root_word();
        throw std::runtime_error(sstr.str());
    };

    boost::iterator_range<trie_t::iterator>
        rng =  trie_transition(fwd,fwd.start(),root_word(), hop_compare());

    if (boost::empty(rng)) return;

    BOOST_FOREACH(trie_t::state const& fwd_root, rng) {
        trie_search( fwd
                   , g
                   , get(boost::edge_bundle,g)
                   , fwd_root
                   , target(ed,g)
                   , back_inserter(fwd_vec)
                   , hop_compare() );
    }
    boost::reverse_graph<Graph,Graph const&> const rg = boost::make_reverse_graph(g);
    trie_search( bkwd
               , rg
               , get(boost::edge_bundle,rg)
               , bkwd.start()
               , source(ed,g)
               , back_inserter(bkwd_vec)
               , hop_compare() );

    lessfirst l1st;
    sort(fwd_vec.begin(),fwd_vec.end(),l1st);
    sort(bkwd_vec.begin(),bkwd_vec.end(),l1st);

    typename search_vec_t::iterator fi = fwd_vec.begin(), fe = fwd_vec.end();

    syntax_key_value_t const* beg = syntax_rules_map.get();
    syntax_key_value_t const* end = beg + syntax_rules_map_sz;

    for (; fi != fe; ++fi) {
        syntax_key_value_t const* ri;
        syntax_key_value_t const* re;
        tie(ri,re) = equal_range(beg,end,boost::get<0>(*fi),left_key_sort());
        beg = re;
        typename search_vec_t::iterator p, b, e;
        b = bkwd_vec.begin();
        e = bkwd_vec.end();
        for (;ri != re; ++ri) {
            p = lower_bound(b,e,ri->key.second,l1st);
            if (p != e and boost::get<0>(*p) == ri->key.second) {
                value_type<Graph> vt;
                vt.c_str = syntax_rules.get() + ri->value_offset;
                vt.len = ri->value_sz;
                vt.left = boost::get<1>(*p);
                vt.right = boost::get<1>(*fi);
                *out = vt;
                ++out;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace xrsdb

# endif //     XRSDB__WORD_CLUSTER_HPP
