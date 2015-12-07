# include <word_cluster.hpp>
# include <collapsed_signature_iterator.hpp>
# include <syntax_rule_util.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <gusc/trie/traverse_trie.hpp>
# include <gusc/trie/set_values_visitor.hpp>
# include <gusc/functional.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
#include <boost/property_map/vector_property_map.hpp>
#else
#include <boost/vector_property_map.hpp>
#endif
# include <iostream>

using namespace sbmt;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

struct get_token {
    typedef indexed_token const& result_type;

    template <class X>
    indexed_token const& operator()(X const& x) const
    {
        return x.get_token();
    }
};

////////////////////////////////////////////////////////////////////////////////

namespace xrsdb {

////////////////////////////////////////////////////////////////////////////////

void word_cluster_construct::swap_self(word_cluster_construct& other)
{
    std::swap(wd,other.wd);
    swap(fwd,other.fwd);
    swap(bkwd,other.bkwd);
    std::swap(fwd_id,other.fwd_id);
    std::swap(bkwd_id,other.bkwd_id);
    syntax_rules_map.swap(other.syntax_rules_map);
    //syntax_keys.swap(other.syntax_keys);
    //syntax_rules.swap(other.syntax_rules);
}

////////////////////////////////////////////////////////////////////////////////

word_cluster_construct::word_cluster_construct(sbmt::indexed_token const& wd)
  : wd(wd)
  , fwd(0)
  , bkwd(0)
  , fwd_id(1)
  , bkwd_id(1)
  //, syntax_keys(1)
  {}

////////////////////////////////////////////////////////////////////////////////

/*
pair<uint32_t const*,uint32_t const*>
word_cluster::rules(sig_key_t const& offset) const
{
    size_t x = syntax_keys[offset];
    return make_pair( syntax_keys.get() + offset + 1
                    , syntax_keys.get() + offset + 1 + x );
}
*/

////////////////////////////////////////////////////////////////////////////////

word_cluster::word_cluster()
  : syntax_rules_map_sz(0)
  , syntax_rules_sz(0) {}

////////////////////////////////////////////////////////////////////////////////

bool word_cluster::operator == (word_cluster const& o) const
{

    return syntax_rules_map_sz == o.syntax_rules_map_sz
       and syntax_rules_sz == o.syntax_rules_sz
       and wd == o.wd
       and equal_trie(fwd,o.fwd)
       and fwd.nonvalue() == o.fwd.nonvalue()
       and equal_trie(bkwd,o.bkwd)
       and bkwd.nonvalue() == o.bkwd.nonvalue()
       and memcmp( syntax_rules_map.get()
                 , o.syntax_rules_map.get()
                 , syntax_rules_map_sz * sizeof(syntax_key_value_t) ) == 0
       and memcmp( syntax_rules.get()
                 , o.syntax_rules.get()
                 , syntax_rules_sz * sizeof(char) ) == 0;

}

size_t word_cluster::memory_size() const
{
    return fwd.memory_size()
         + bkwd.memory_size()
         + sizeof(syntax_key_value_t) * syntax_rules_map_sz
         + sizeof(char) * syntax_rules_sz ;
}

void word_cluster::dump(std::ostream& os) const
{
    const char* rules = syntax_rules.get();
    size_t sz = 0;

    while (rules < syntax_rules.get() + syntax_rules_sz) {
        sz = strlen(rules);
        os.write(rules,sz);
        rules = rules + sz + 1;
    }
}

////////////////////////////////////////////////////////////////////////////////

word_cluster::word_cluster(word_cluster_construct const& t)
  : syntax_rules_map_sz(t.syntax_rules_map.size())
  , syntax_rules_sz(0)
  , syntax_rules_map(new syntax_key_value_t[syntax_rules_map_sz])
  , wd(t.wd)
  , fwd(t.fwd)
  , bkwd(t.bkwd)
{
    word_cluster_construct::syntax_rules_map_t::const_iterator
        itr = t.syntax_rules_map.begin(),
        end = t.syntax_rules_map.end();

    for (size_t skm_offset = 0; itr != end; ++itr, ++skm_offset) {
        uint_type value_sz = 0;
        syntax_rules_map[skm_offset].key = itr->first;
        syntax_rules_map[skm_offset].value_offset = syntax_rules_sz;
        vector<string>::const_iterator i = itr->second.begin(),
                                       e = itr->second.end();
        for (; i != e; ++i) {
            uint_type sz = i->size() + 1;
            syntax_rules_sz += sz;
            value_sz += sz;
        }
        syntax_rules_map[skm_offset].value_sz = value_sz;
        ++syntax_rules_sz; // for the null terminal
    }

    syntax_rules.reset(new char[syntax_rules_sz]);
    itr = t.syntax_rules_map.begin();
    char* sr = syntax_rules.get();

    for (; itr != end; ++itr) {
        vector<string>::const_iterator
            i = itr->second.begin(),
            e = itr->second.end();
        for (; i != e; ++i) {
            copy(i->begin(),i->end(),sr);
            sr += i->size();
            *sr = '\n';
            ++sr;
        }
        *sr = 0;
        ++sr;
    }
    sort( syntax_rules_map.get()
        , syntax_rules_map.get() + syntax_rules_map_sz
        , key_sort()
        )
        ;
}
sbmt::indexed_token kwd(sbmt::indexed_token tok, sbmt::wildcard_array& wc) {
    if (tok.type() == sbmt::virtual_tag_token) {
        tok = wc[0];
    }
    return tok;
}
////////////////////////////////////////////////////////////////////////////////

cluster_search::cluster_search(
  header const& h
, word_trie const* trie
, Graph const* g
, boost::graph_traits<Graph>::vertex_descriptor b
)
: trie(trie)
, g(g)
, ph(&h)
{
    states.push_back(state(backtrack_type(),trie->start(),b));
}

////////////////////////////////////////////////////////////////////////////////

cluster_search::cluster_search( 
  header const& h
, word_trie const* trie
, Graph const* g
, boost::graph_traits<Graph>::vertex_descriptor b
, boost::graph_traits<Graph>::edge_descriptor e 
)
: trie(trie)
, g(g)
, ph(&h)
{
    //indexed_token rw = (*g)[e];
    //if (rw.type() == sbmt::virtual_tag_token) {
    //    states.push_back(state(backtrack_type(),trie->start(),b));
    //}
    //else {
        span_t ispn((*g)[source(e,*g)],(*g)[target(e,*g)]);
        std::vector<state> processed;
        states.push_back(state(backtrack_type(1,boost::make_tuple(ispn,1)),trie->start(),source(e,*g)));
        assert((*g)[states.front().get<2>()] >= (*g)[b]);
    
        while (not states.empty()) {
            input_state in; output_state out;
            backtrack_type bt;
            boost::tie(bt,in,out) = states.back();
            states.pop_back();
            if (out == b) {
                state tostate(bt,in,target(e,*g));
                processed.push_back(tostate);
            } else if ((*g)[out] < (*g)[b]) {
                continue;
            } else {
                BOOST_FOREACH( boost::graph_traits<Graph>::edge_descriptor t
                             , in_edges(out,*g) ) {
                    sbmt::indexed_token otok = (*g)[t];
                    output_state o = source(t,*g);
                    BOOST_FOREACH(input_state to, trie->transitions(in)) {
                        short dir; sbmt::indexed_token itok;
                        boost::tie(dir,itok) = trie->key(to);
                        // verify the transition is compatible:
                        if (dir > 0) continue; // wrong way!
                        if (itok.type() != otok.type()) continue;
                        if (itok.type() == sbmt::foreign_token and itok != otok) continue;
                        if (itok.index() > otok.index()) continue;
                        size_t n = 0;
                        sbmt::span_t spn;
                        if (itok.type() == sbmt::virtual_tag_token) {
                            n = boost::lexical_cast<size_t>(ph->dict.label(itok));
                            spn = sbmt::span_t((*g)[source(t,*g)],(*g)[target(t,*g)]);
                            //std::cerr << "should be a push of " << spn << "\n";
                            backtrack_type bbtt = bt;
                            bbtt.push_back(boost::make_tuple(spn,n));
                            states.push_back(state(bbtt,to,o));
                        } else {
                            //std::cerr << "should be a push of " << itok << "\n";
                            n = 1;
                            spn = sbmt::span_t((*g)[source(t,*g)],(*g)[target(t,*g)]);
                            backtrack_type bbtt = bt;
                            bbtt.push_back(boost::make_tuple(spn,n));
                            states.push_back(state(bbtt,to,o));
                            //states.push_back(state(bt,to,o));
                        }
                    }
                }
            }
        }
        processed.swap(states);
    //}
    /*
    std::cerr << (*g)[b] 
              << "...[" 
              << (*g)[source(e,*g)] 
              << " -- " 
              << (*g)[e] 
              << " -- " 
              << (*g)[target(e,*g)] 
              <<  "]\n";
     */
}


cluster_search::result_type
cluster_search::operator()(output_state f)
{
    std::vector<state> processed;
    result_type res;
    
    while (not states.empty()) {
        input_state in; output_state out; backtrack_type bt;
        boost::tie(bt,in,out) = states.back();
        states.pop_back();
        if ((*g)[out] >= (*g)[f]) {
            processed.push_back(state(bt,in,out));
            if (out == f) {
                if (trie->value(in) != trie->nonvalue()) {
                    std::sort(bt.begin(),bt.end());
                    res.push_back(boost::make_tuple(bt,trie->value(in)));
                }
            }
        } else {
            BOOST_FOREACH( boost::graph_traits<Graph>::edge_descriptor t
                         , out_edges(out,*g) ) {
                sbmt::indexed_token otok = (*g)[t];
                //output_state o = source(t,*g);
                BOOST_FOREACH(input_state to, trie->transitions(in)) {
                    short dir; sbmt::indexed_token itok;
                    boost::tie(dir,itok) = trie->key(to);
                    // verify the transition is compatible:
                    if (dir < 0) continue; // wrong way!
                    if (itok.type() != otok.type()) continue;
                    if (itok.type() == sbmt::foreign_token and itok != otok) continue;
                    if (itok.index() > otok.index()) continue;
                    
                    size_t n = 0;
                    sbmt::span_t spn;
                    if (itok.type() == sbmt::virtual_tag_token) {
                        n = boost::lexical_cast<size_t>(ph->dict.label(itok));
                        spn = sbmt::span_t((*g)[source(t,*g)],(*g)[target(t,*g)]);
                        //std::cerr << "should be a push of " << spn << "\n";
                        backtrack_type bbtt = bt;
                        bbtt.push_back(boost::make_tuple(spn,n));
                        states.push_back(state(bbtt,to,target(t,*g)));
                    } else {
                        //std::cerr << "should be a push of " << itok << "\n";
                        n = 1;
                        spn = sbmt::span_t((*g)[source(t,*g)],(*g)[target(t,*g)]);
                        backtrack_type bbtt = bt;
                        bbtt.push_back(boost::make_tuple(spn,n));
                        states.push_back(state(bbtt,to,target(t,*g)));
                        //states.push_back(state(bt,to,target(t,*g)));
                    }
                }
            }
        }
    }
    processed.swap(states);
    //std::cerr << "searchdb result.size() == "<< res.size() << '\n';
    return res;
}
} // namespace xrsdb

