#if 0
# include <boost/archive/binary_iarchive.hpp>
# include <boost/archive/binary_oarchive.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/test/auto_unit_test.hpp>
# include <gusc/trie/basic_trie.hpp>
# include <gusc/trie/fixed_trie.hpp>
# include <gusc/trie/trie_algo.hpp>
# include <gusc/trie/sentence_lattice.hpp>
# include <set>
# include <string>
# include <iostream>
# include <fstream>
# include <boost/graph/adjacency_list.hpp>
# include <boost/function_output_iterator.hpp>
# include <boost/graph/graphviz.hpp>

using namespace std;
using namespace boost;
using namespace gusc;

typedef property<edge_name_t,char> edge_property_t;
typedef property<vertex_index_t,size_t> vertex_property_t;
typedef adjacency_list<vecS,vecS,directedS,vertex_property_t,edge_property_t> 
        token_graph_t;
        
namespace {

////////////////////////////////////////////////////////////////////////////////

struct insert_results {
    insert_results(std::set<size_t>& mr) : match_results(&mr) {}
    std::set<size_t>* match_results;
    template <class ResultType>
    void operator()(ResultType const& r) 
    { 
        match_results->insert(boost::get<0>(r));
    }
};

struct gen_ex {
    typedef char value_type;
    char operator[](size_t) const { return 'x'; }
};

template <class Trie, class I>
void expected_search(Trie const& trie, string match, I ebeg, I eend)
{
    token_graph_t token_graph;
    boost::graph_traits<token_graph_t>::vertex_descriptor start, finish;
    boost::tie(start,finish) = skip_lattice_from_sentence( token_graph
                                                         , match.begin()
                                                         , match.end()
                                                         , gen_ex()
                                                         );
    /*                                        
    write_graphviz(
      std::cout
    , token_graph
    , boost::default_writer()
    , boost::make_label_writer(get(edge_name_t(),token_graph))
    )
    ;
    std::cout << std::endl;
    */
                                                     
    set<size_t> match_results;
    trie_search( trie
               , token_graph
               , start
               , boost::make_function_output_iterator(insert_results(match_results))
               );
               
    BOOST_CHECK_EQUAL_COLLECTIONS( match_results.begin(), match_results.end()
                                 , ebeg, eend );
}

////////////////////////////////////////////////////////////////////////////////

string match = "abcde";
const size_t s_sz = 9;
string s[s_sz] = { "abcd"  // 1
                 , "abcx"  // 1
                 , "xbcde" // 1
                 , "bxde"  // 0
                 , "axcd"  // 1 
                 , "axbc"  // 0
                 , "xcx"   // 1
                 , "xcxd"  // 0
                 , "axc"   // 1
                 };
const size_t exp_sz = 6;
size_t expected[exp_sz] = { 1, 2, 3, 5, 7, 9 };

////////////////////////////////////////////////////////////////////////////////

fixed_trie<char,size_t> build_trie(string const* itr, string const* end)
{
    basic_trie<char,size_t> trie(0);
    size_t x = 1;
    for (; itr != end; ++x, ++itr) {
        trie.insert(itr->begin(),itr->end(),x);
    }
    return fixed_trie<char,size_t>(trie);
}

////////////////////////////////////////////////////////////////////////////////

} // unnamed namespace

BOOST_AUTO_TEST_CASE(test_trie_search)
{   
    fixed_trie<char,size_t> trie = build_trie(s, s + s_sz);
    expected_search(trie,match,expected,expected + exp_sz);
    //copy(match_results.begin(),match_results.end(),ostream_iterator<size_t>(cerr," "));
}

BOOST_AUTO_TEST_CASE(test_trie_copy)
{
    fixed_trie<char,size_t> trie = build_trie(s, s + s_sz);
    fixed_trie<char,size_t> trie2 = trie;
    expected_search(trie2,match,expected,expected + exp_sz);
    fixed_trie<char,size_t> trie3(trie);
    expected_search(trie3,match,expected,expected + exp_sz);
}

BOOST_AUTO_TEST_CASE(test_trie_eq)
{
    fixed_trie<char,size_t> trie = build_trie(s, s + s_sz);
    fixed_trie<char,size_t> trie2 = trie;
    BOOST_CHECK(equal_trie(trie,trie2));
    fixed_trie<char,size_t> trie3 = build_trie(s, s + s_sz);
    BOOST_CHECK(equal_trie(trie3,trie));
}

BOOST_AUTO_TEST_CASE(test_trie_serialize)
{
    fixed_trie<char,size_t> origtrie = build_trie(s,s + s_sz);
    expected_search(origtrie,match,expected,expected + exp_sz);
    {
        fixed_trie<char,size_t> trie = build_trie(s,s + s_sz);
        std::ofstream out("test_trie_serialize");
        boost::archive::binary_oarchive arout(out);
        arout & trie;
        BOOST_CHECK(equal_trie(origtrie,trie));
    }
    // time passes...
    fixed_trie<char,size_t> trie;
    //BOOST_CHECK(not equal_trie(origtrie,trie));
    {
        std::ifstream in("test_trie_serialize");
        boost::archive::binary_iarchive arin(in);
        arin & trie;
    }
    
    BOOST_CHECK(equal_trie(origtrie,trie));
    BOOST_CHECK(origtrie.nonvalue() == trie.nonvalue());
    
    boost::filesystem::remove("test_trie_serialize");
    expected_search(origtrie,match,expected,expected + exp_sz);
    expected_search(trie,match,expected,expected + exp_sz);

}
#endif