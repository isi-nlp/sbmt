namespace xrsdb {

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

template <class Graph, class PMap, class OutIterator>
void 
word_cluster::search( Graph const& g
                    , PMap const& pmap
                    , typename boost::graph_traits<Graph>::edge_descriptor ed
                    , OutIterator out ) const
{
    using namespace std;
    using namespace boost;
    vector<uint32_t> fwd_vec, bkwd_vec;
    assert(get(pmap,ed) == root_word());

    bool present;
    trie_t::state fwd_root;
    tie(present,fwd_root) = trie_transition(fwd,fwd.start(),root_word());

    if (not present) return;

    trie_search( fwd
               , g
               , pmap
               , fwd_root
               , target(ed,g)
               , back_inserter(fwd_vec) );

    trie_search( bkwd
               , boost::make_reverse_graph(g)
               , pmap
               , bkwd.start()
               , source(ed,g)
               , back_inserter(bkwd_vec) );

    sort(fwd_vec.begin(),fwd_vec.end());
    sort(bkwd_vec.begin(),bkwd_vec.end());

    vector<uint32_t>::iterator fi = fwd_vec.begin(), fe = fwd_vec.end();

    syntax_key_value_t const* beg = syntax_rules_map.get();
    syntax_key_value_t const* end = beg + syntax_rules_map_sz;                   

    for (; fi != fe; ++fi) {
        syntax_key_value_t const* ri;
        syntax_key_value_t const* re;
        tie(ri,re) = equal_range(beg,end,*fi,left_key_sort());
        beg = re;
        vector<uint32_t>::iterator p, b, e;
        b = bkwd_vec.begin();
        e = bkwd_vec.end();
        for (;ri != re; ++ri) {
            p = lower_bound(b,e,ri->key.second);
            if (p != e and *p == ri->key.second) {
                value_type vt;
                vt.c_str = syntax_rules.get() + ri->value_offset;
                vt.len = ri->value_sz;
                *out = vt;
                ++out;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace xrsdb
