#include <boost/test/auto_unit_test.hpp>
#include <sbmt/grammar/lm_string.hpp>
#include <vector>
/*
BOOST_AUTO_TEST_CASE(test_fat_lm_string)
{
    using namespace sbmt;
    
    fat_token_factory tf;
    
    fat_lm_string lms("\"hello\" 1 x0 \"world\"",tf);
    BOOST_CHECK_EQUAL(lms.size(),4);
    
    BOOST_CHECK(lms[0].is_token() == true);
    BOOST_CHECK(lms[0] == tf.native_word("hello"));
    BOOST_CHECK(lms[1].is_token() == false);
    BOOST_CHECK(lms[1] == 1);
    BOOST_CHECK(lms[2].is_token() == false);
    BOOST_CHECK(lms[2] == 0);
    BOOST_CHECK(lms[3].is_token() == true);
    BOOST_CHECK(lms[3] == tf.native_word("world"));
    
    fat_lm_string::const_iterator itr = lms.begin();
    fat_lm_string::const_iterator end = lms.end();
    
    BOOST_CHECK(*itr == native_word("hello"));
    ++itr;
    BOOST_CHECK(*itr == 1);
    ++itr;
    BOOST_CHECK(*itr == 0);
    ++itr;
    BOOST_CHECK(*itr == native_word("world"));
    ++itr;
    BOOST_CHECK(itr == end);
}
*/

BOOST_AUTO_TEST_CASE(test_indexed_lm_string)
{
    using namespace sbmt;
    using std::vector;
    
    in_memory_dictionary dict;
//    indexed_token_factory tf = dict;
    
    indexed_lm_string lms("\"hello\" 1 x0 \"world\" \"\"\"",dict);
    
    BOOST_CHECK(lms.size() == 5);
    BOOST_CHECK(lms[0].is_token() == true);
    BOOST_CHECK(lms[0] == dict.native_word("hello"));
    BOOST_CHECK(lms[1].is_token() == false);
    BOOST_CHECK(lms[1] == 1);
    BOOST_CHECK(lms[2].is_token() == false);
    BOOST_CHECK(lms[2] == 0);
    BOOST_CHECK(lms[3].is_token() == true);
    BOOST_CHECK(lms[3] == dict.native_word("world"));
    
    indexed_lm_string::const_iterator itr = lms.begin();
    indexed_lm_string::const_iterator end = lms.end();
    
    vector<indexed_token> v;
    
    BOOST_CHECK(*itr == dict.native_word("hello"));
    v.push_back(itr->get_token());
    ++itr;
    BOOST_CHECK(itr->is_token() == false);
    BOOST_CHECK(*itr == 1);
    ++itr;
    BOOST_CHECK(itr->is_token() == false);
    BOOST_CHECK(*itr == 0);
    ++itr;
    BOOST_CHECK(*itr == dict.native_word("world"));
    v.push_back(itr->get_token());
    ++itr;
    BOOST_CHECK(*itr == dict.native_word("\""));
    ++itr;
    BOOST_CHECK(itr == end);
}
