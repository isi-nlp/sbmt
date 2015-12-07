/*# include <sbmt/search/unary_applications.hpp>
# include <sbmt/edge/null_info.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/grammar/brf_file_reader.hpp>

# include <fstream>
# include <sstream>

# include <boost/test/auto_unit_test.hpp>
# include <boost/algorithm/string/trim.hpp>

using namespace sbmt;
using namespace std;

template class unary_applications< edge<null_info>, grammar_in_mem >;

void compare_edges_from_foo(string const& filebase)  
{
    typedef edge<null_info> edge_t;
    typedef edge_equivalence<edge_t> edge_equiv_t;
    edge_factory<edge_t> ef;
    grammar_in_mem gram;
    
    edge_equiv_t::max_edges_per_equivalence(ULONG_MAX);
    
    score_combiner sc("scr:1.0");
    ifstream file((SBMT_TEST_DIR + string("/") + filebase + ".brf").c_str());
    ifstream edges((SBMT_TEST_DIR + string("/") + filebase + ".edges").c_str());
    brf_stream_reader brf(file);
    gram.load(brf,sc);
    
    edge_equiv_t eq(new edge_equivalence_impl<edge_t>(ef.create_edge(gram,"FOO",span_t(0,1))));
    std::vector<edge_equiv_t> v;
    v.push_back(eq);
    unary_applications<edge_t,grammar_in_mem> u(v.begin(), v.end(), gram, ef, true);
    unary_applications<edge_t,grammar_in_mem>::iterator itr = u.begin(),
                                                        end = u.end();
                                                        
    multiset<string> actual, results;
    for (; itr != end; ++itr) {
        edge_equiv_t::edge_range er = const_cast<edge_equiv_t&>(*itr).edges_sorted();
        edge_equiv_t::edge_iterator eitr = er.begin(), eend = er.end();
        for (; eitr != eend; ++eitr) {
            stringstream sstr;
            sstr << print(*eitr,gram);
            results.insert(boost::trim_copy(sstr.str()));
        }
    }
    while (edges) {
        string line;
        getline(edges,line);
        if (line != "") actual.insert(boost::trim_copy(line));
    }
    BOOST_CHECK_EQUAL(actual.size(),results.size());
    BOOST_CHECK_EQUAL_COLLECTIONS( actual.begin(), actual.end()
                                 , results.begin(), results.end() );
                                 
    for (multiset<string>::iterator ai = actual.begin(); ai != actual.end(); ++ai) {
        cerr << *ai << endl;
    }
    cerr << "================================" << endl;
    for (multiset<string>::iterator ai = results.begin(); ai != results.end(); ++ai) {
        cerr << *ai << endl;
    }
}

BOOST_AUTO_TEST_CASE(test_unary_applications) 
{
    compare_edges_from_foo("unary1");
    compare_edges_from_foo("unary2");
    compare_edges_from_foo("unary3");
}*/

   

