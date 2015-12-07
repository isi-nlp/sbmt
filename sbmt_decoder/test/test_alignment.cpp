#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/grammar/alignment.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <boost/test/auto_unit_test.hpp>

#include "test_util.hpp"
#include <graehl/shared/test.hpp>
#include "grammar_examples.hpp"

BOOST_AUTO_TEST_CASE(test_alignment)
{
    using namespace sbmt;
    using namespace std; 

    alignment a;
    a.set_empty(3,4);
    a.add(0,3);
    a.add(1,3);
    a.add(1,2);
    a.add(2,0);

    string a_str="[#s=3 #t=4 0,3 1,1 1,2 2,0]";
    BOOST_CHECK(graehl::test_extract_insert(a_str,a));
    
    alignment b;
    b.set_inverse(a);
    string b_str="[#s=4 #t=3 0,2 1,1 2,1 3,0]";
    BOOST_CHECK(graehl::test_extract_insert(b_str,b));

    alignment c(b,as_inverse());

    BOOST_CHECK_EQUAL(a,c);

    alignment zero("[#s=0 #t=0 ]");
    BOOST_CHECK_EQUAL(zero,alignment());
    
    alignment one("[#s=1 #t=1 0,0]");
    BOOST_CHECK_EQUAL(one,alignment(as_unit()));

    alignment two("[#s=2 #t=2 0,0 1,1]");
    
    typedef alignment::substitution S;
    vector<S> s0,s1,s2,sa,snone;
    s0.push_back(S(zero,1));
    alignment a0(a,s0);
    
    BOOST_CHECK_EQUAL(alignment(a,s0),alignment("[#s=2 #t=2 0,1 1,0]")); // deletes 1 from s; 1,2 from t
    s1.push_back(S(one,1));
    BOOST_CHECK_EQUAL(alignment(a,s1),a); //identity
    BOOST_CHECK_EQUAL(alignment(b,s1),b);
    BOOST_CHECK_EQUAL(alignment(a0,s1),a0);
    
    s2.push_back(S(two,1));
    
    BOOST_CHECK_EQUAL(alignment(a,s2),alignment("[#s=4 #t=6 0,5 1,1 1,3 2,2 2,4 3,0]")); // each of 1->1 and 1->2 becomes [2x2]
    sa.push_back(S(a,2));

    BOOST_CHECK_EQUAL(alignment(a,sa),alignment("[#s=5 #t=7 0,6 1,4 1,5 2,3 3,1 3,2 4,0]"));

    BOOST_CHECK_EQUAL(alignment(a,snone),a);
    BOOST_CHECK_EQUAL(alignment(zero,snone),zero);
}

BOOST_AUTO_TEST_CASE(test_syntax_rule_default_alignment)
{
    using namespace sbmt;
    using namespace std;
    
    grammar_in_mem g;
    init_grammar_marcu_staggard_wts(g);
    grammar_in_mem::rule_range r = g.all_rules();
    for (grammar_in_mem::rule_iterator i=r.begin(),e=r.end();i!=e;++i) {
        syntax_id_type id=g.get_syntax_id(*i);
        if (id!=NULL_SYNTAX_ID) {
            
            indexed_syntax_rule const& syn=g.get_syntax(id);
        
            unsigned ns=syn.rhs_size(),nt=syn.lhs_yield_size();

            alignment a_empty=syn.default_alignment(false);
            BOOST_CHECK_EQUAL(a_empty.n_src(),ns);
            BOOST_CHECK_EQUAL(a_empty.n_tar(),nt);

            alignment a_full=syn.default_alignment(true);
            BOOST_CHECK_EQUAL(a_full.n_src(),ns);
            BOOST_CHECK_EQUAL(a_full.n_tar(),nt);
        }
    }
}
