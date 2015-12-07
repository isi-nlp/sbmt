#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <sbmt/token/in_memory_token_storage.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <sstream>

using namespace sbmt;
using namespace boost;
using namespace archive;

BOOST_AUTO_TEST_CASE(test_in_memory_token_storage)
{
    in_memory_token_storage dict, dict2;
    typedef in_memory_token_storage::index_type index_type;
    
    BOOST_CHECK_EQUAL(dict.size(),0u);
    std::string s1="asdf",s2="asdf2",s3="23";
    index_type i1,i2,i3;
    BOOST_CHECK_EQUAL(i1=dict.get_index(s1),0u);
    BOOST_CHECK_EQUAL(dict.get_index(s1),i1);
    BOOST_CHECK_EQUAL(dict.size(),1u);
    BOOST_CHECK_EQUAL(s1,dict.get_token(i1));
    
    std::stringstream sstr;
    text_oarchive oa(sstr);
    oa & dict;
    
    text_iarchive ia(sstr);
    ia & dict2;

    BOOST_CHECK_EQUAL(dict2.get_index(s1),i1);
    BOOST_CHECK_EQUAL(s1,dict2.get_token(i1));
    BOOST_CHECK_EQUAL(i2=dict2.get_index(s2),1u);
    BOOST_CHECK_EQUAL(dict2.get_index(s1),i1);
    BOOST_CHECK_EQUAL(i3=dict2.get_index(s3),2u);
    BOOST_CHECK_EQUAL(dict2.get_index(s1),i1);
    BOOST_CHECK_EQUAL(dict2.get_index(s2),i2);
    BOOST_CHECK_EQUAL(s2,dict2.get_token(i2));
}

