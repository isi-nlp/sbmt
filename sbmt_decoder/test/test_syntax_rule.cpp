#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <boost/lexical_cast.hpp>
#include <sbmt/grammar/syntax_rule.hpp>
#include <sbmt/token.hpp>
#include <iostream>
#include <string>
#include <boost/test/auto_unit_test.hpp>

#include "test_util.hpp"

static std::string syntax_rule_str = 
    "S(NP-C(x0:NPB) VP(MD(\"will\") VP-C(VB(\"be\") x1:VP-C)) .(\".\")) -> "
    "x0 \"A\" \"B\" x1 \".\" ### id=9";

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(test_syntax_rule)
{
    using namespace sbmt;
    using namespace std; 
        
    fat_token_factory tf;
    
    syntax_rule<fat_token> r(syntax_rule_str,tf);
    
    BOOST_CHECK_EQUAL(r.lhs_root()->get_token(),tag("S"));

    BOOST_CHECK_EQUAL(r.n_native_tokens(),3u);
    
    BOOST_CHECK_EQUAL(r.n_leaves(),5u);
    
    fat_token children[] = {tag("NP-C"), tag("VP"), tag(".")};
    
    fat_token const* cbegin = children;
    fat_token const* cend   = children + 3;
    BOOST_CHECK_EQUAL_COLLECTIONS(
        cbegin, cend,
        r.lhs_root()->children_begin(), r.lhs_root()->children_end()
    );
    
    BOOST_CHECK_EQUAL(syntax_rule_str,to_string(r,tf));
    
    stringstream strstr;
    boost::archive::binary_oarchive oa(strstr);
    oa & r;
    
    boost::archive::binary_iarchive ia(strstr);
    ia & r;
    
    BOOST_CHECK_EQUAL(syntax_rule_str,to_string(r,tf));
}

BOOST_AUTO_TEST_CASE(fatten_index_syntax_rule)
{
    using namespace sbmt;

    in_memory_dictionary dict;
    fat_token_factory tf;
    
    fat_syntax_rule rf(syntax_rule_str,tf);
    indexed_syntax_rule ri(syntax_rule_str,dict);
    
    std::cout << "        rf=" << print(rf,tf) << std::endl;
    std::cout << "fatten(ri)=" << print(fatten(ri,dict),tf) << std::endl;
    BOOST_CHECK(rf == fatten(ri,dict));
    BOOST_CHECK(ri == index(rf,dict));
    
}
