# include <boost/archive/binary_oarchive.hpp>
# include <boost/archive/binary_iarchive.hpp>
# include <boost/test/auto_unit_test.hpp>
# include <string>
# include <sstream>
# include <gusc/trie/sentence_lattice.hpp>
# include <boost/graph/adjacency_list.hpp>
# include <word_cluster.hpp>
# include <syntax_rule_util.hpp>
# include <collapsed_signature_iterator.hpp>
# include <sbmt/sentence.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/graph/graphviz.hpp>
# include <lattice_reader.hpp>

using namespace xrsdb;
using namespace std;
using namespace gusc;
using namespace sbmt;
using namespace boost;
using namespace boost::archive;

typedef property<edge_name_t,indexed_token> edge_property_t;
typedef property<vertex_index_t,size_t> vertex_property_t;
typedef adjacency_list<listS,listS,bidirectionalS,vertex_property_t,edge_property_t> 
        graph_t;


namespace {

////////////////////////////////////////////////////////////////////////////////

template <class I>
word_cluster build_word_cluster(I itr, I end, string w, indexed_token_factory& tf)
{
    indexed_token skip = tf.tag("x");
    indexed_token wd = tf.foreign_word(w);
    
    word_cluster_construct cons(wd);
    for (;itr != end; ++itr) { 
        const vector<indexed_token> rhs = rhs_from_string(*itr,tf);
        cons.insert(rhs, *itr); 
    }
    word_cluster db(cons);
    
    return db;
}

////////////////////////////////////////////////////////////////////////////////

template <class I2>
void expected_retrieval(word_cluster const& db, string s, I2 ei, I2 ee, indexed_token_factory& tf)
{
    using sbmt::graph_t;
    set<string> expected(ei,ee);
    set<string> actual;
    
    indexed_sentence sent = foreign_sentence(s,tf);
    indexed_token skip = tf.tag("x");
    
    graph_t g;
    skip_lattice_from_sentence(g,sent.begin(),sent.end(),wildcard_array(tf));
    
    graph_t::vertex_iterator vi, ve;
    tie(vi,ve) = vertices(g);
    size_t id = 0;
    for(;vi != ve; ++vi) { put(vertex_index_t(),g,*vi,id++); }
    clog << token_label(tf);
    write_graphviz( clog
                  , g
                  , default_writer()
                  , make_label_writer(get(edge_name_t(),g))
                  , default_writer() );
    
    write_graphviz( clog
                  , make_reverse_graph(g)
                  , default_writer()
                  , make_label_writer(get(edge_name_t(),g))
                  , default_writer() );
    
    typedef graph_traits<graph_t>::edge_iterator eitr_t;
    eitr_t eitr,eend;
    tie(eitr,eend) = edges(g);
    
    stringstream sstr;
    
    for (;eitr != eend; ++eitr) {
        sbmt::indexed_token etok = get(edge_name_t(),g,*eitr);
        sbmt::indexed_token rtok = db.root_word();
        if (etok == rtok) {
            db.search( g
                     , get(edge_name_t(),g)
                     , *eitr
                     , ostream_iterator< word_cluster::value_type<graph_t> >(sstr,"")
                     //, inserter(actual,actual.end())
                     )
                     ;
        }
    }
    string line;
    while(getline(sstr,line)) {
        actual.insert(line);
    }
    
    BOOST_CHECK_EQUAL_COLLECTIONS( expected.begin(), expected.end()
                                 , actual.begin(), actual.end() );
}

////////////////////////////////////////////////////////////////////////////////

size_t const rules_size = 6;
string rules[rules_size] = {
    "X(x0:X x1:X) -> \"a\" \"b\" x0 x1 \"d\" ### id=1"  // no
  , "X(x0:X x1:X) -> x0 x1 \"b\" \"c\" \"d\" ### id=2"  // no
  , "X(x0:X) -> \"b\" \"b\" x0 ### id=3" // no
  , "X(\"A\") -> \"a\" \"b\" ### id=4" // yes
  , "X(\"B\") -> \"b\" ### id=5" // yes
  , "X(x0:X) -> \"b\" x0 \"d\" ### id=6" // yes
};

string word = "b";
string sent = "a b c d e";

size_t const expected_size = 3;
string expected[expected_size] = { rules[3]
                                 , rules[4]
                                 , rules[5] };

////////////////////////////////////////////////////////////////////////////////

size_t const rules2_size = 6;
string rules2[rules2_size] = {
    "X(x0:X x1:X) -> \"a\" \"b\" x0 x1 \"d\" ### id=1"  // no
  , "X(x0:X x1:X) -> x0 x1 \"b\" \"c\" \"d\" ### id=2"  // no
  , "X(x0:X) -> \"d\" \"d\" x0 ### id=3" // no
  , "X(\"A\") -> \"c\" \"d\" ### id=4" // yes
  , "X(\"B\") -> \"d\" ### id=5" // yes
  , "X(x0:X) -> \"b\" x0 \"d\" ### id=6" // yes
};

string word2 = "d";
string sent2 = "b c d e";
size_t const expected2_size = 3;
string expected2[expected2_size] = { rules2[3]
                                   , rules2[4]
                                   , rules2[5] };

////////////////////////////////////////////////////////////////////////////////

} // unnamed namespace

BOOST_AUTO_TEST_CASE(test_wc_retrieval)
{
    std::cout << "test_wc_regrieval\n"; 
    indexed_token_factory tf;
    word_cluster wc = build_word_cluster(rules, rules + rules_size, word, tf);
    expected_retrieval(wc, sent, expected, expected + expected_size, tf);
}

BOOST_AUTO_TEST_CASE(test_wc_retrieval2)
{
    indexed_token_factory tf;
    word_cluster wc = build_word_cluster(rules2, rules2 + rules2_size, word2, tf);
    expected_retrieval(wc, sent2, expected2, expected2 + expected2_size, tf);
}

BOOST_AUTO_TEST_CASE(test_wc_serialize)
{
    indexed_token_factory tf;
    {
        std::ofstream out("test_cluster_serialize");
        boost::archive::binary_oarchive arout(out);
        word_cluster wc = build_word_cluster(rules, rules + rules_size, word, tf);
        arout & wc;
    }
    // time passes...
    word_cluster wc;
    {
        std::ifstream in("test_cluster_serialize");
        boost::archive::binary_iarchive arin(in);
        arin & wc;
    }
    boost::filesystem::remove("test_cluster_serialize");
    word_cluster orig = build_word_cluster(rules, rules + rules_size, word, tf);
    BOOST_CHECK(wc == orig);
    BOOST_CHECKPOINT("does one work?");
    expected_retrieval(orig, sent, expected, expected + expected_size, tf);
    BOOST_CHECKPOINT("one works");
    expected_retrieval(wc, sent, expected, expected + expected_size, tf);
} 