#include <boost/test/auto_unit_test.hpp>
#include <sbmt/sentence.hpp>

BOOST_AUTO_TEST_CASE(test_sentence)
{
    using namespace sbmt;
    fat_sentence s1(native_token);
    
    s1.push_back(native_word("hello"));
    s1.push_back(native_word("world"));
    
    BOOST_CHECKPOINT("s1 built");
    
    fat_sentence s2(native_token);
    s2.push_front(native_word("there"));
    s2.push_front(native_word("hi"));
    
    BOOST_CHECK(s1 != s2);
    
    BOOST_CHECKPOINT("s2 built");
    
    fat_sentence s3(native_token);
    s3.push_back(native_word("hello"));
    s3.push_back(native_word("world"));
    s3.push_back(native_word("hi"));
    s3.push_back(native_word("there"));
    
    BOOST_CHECKPOINT("s3 built");
    
    fat_sentence s4 = s1 + s2;
    
    BOOST_CHECKPOINT("s4 built");
    
    BOOST_CHECK_EQUAL(s4,s3);
    
    s4 = s1;
    s4 += s2;
    
    BOOST_CHECK_EQUAL(s4,s3);
    
    fat_sentence s5 = s2;
    s5.prepend(s1);
    
    BOOST_CHECKPOINT("s5 built");
    
    BOOST_CHECK_EQUAL(s5,s3);
    
    BOOST_CHECK_THROW(s4 += foreign_word("GUTENTAG"),mismatched_sentence);
    BOOST_CHECK_THROW(s4.push_back(foreign_word("VIE")),mismatched_sentence);
    BOOST_CHECK_THROW(s4.push_front(foreign_word("HALO")),mismatched_sentence);
    
    BOOST_CHECK_EQUAL(s4,s3);
    
    fat_sentence fs(foreign_token);
    fs += foreign_word("GUTENTAG");
    fs += foreign_word("VIE");
    fs += foreign_word("GEHTS");
    
    BOOST_CHECK_THROW(s3 + fs,mismatched_sentence);
    BOOST_CHECK_THROW(s3.append(fs),mismatched_sentence);
    BOOST_CHECK_THROW(s3.prepend(fs),mismatched_sentence);
    
    BOOST_CHECK_EQUAL(s4,s3);
    
    BOOST_CHECKPOINT("all tests run");
}

BOOST_AUTO_TEST_CASE(index_fatten_sentence)
{
    using namespace sbmt;
    in_memory_dictionary dict;
    
    fat_sentence sf = native_sentence("a b c");
    indexed_sentence si = native_sentence("a b c", dict);
    
    BOOST_CHECK( sf == fatten(si,dict) );
    BOOST_CHECK( si == index(sf,dict) );
}
