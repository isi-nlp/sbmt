#include <boost/test/auto_unit_test.hpp>
#include <sbmt/token/tokenizer.hpp>
#include <sbmt/sentence.hpp>

BOOST_AUTO_TEST_CASE(test_tokenizer)
{
    using namespace sbmt;
    
    std::string s = "this is a sentence";
    
    fat_token_factory tf ;
    tokenizer<> tok(s,char_separator<char,fat_token_factory>(tf,native_token));
    
    fat_sentence ss(native_token);
    ss += native_word("this");
    ss += native_word("is");
    ss += native_word("a");
    ss += native_word("sentence");
    
    BOOST_CHECK_EQUAL_COLLECTIONS(ss.begin(),ss.end(),tok.begin(),tok.end());
    
    s = "this: is a sentence, with punctuation.";
    
    tok.assign(s);
    
    ss.clear();
    ss += native_word("this:");
    ss += native_word("is");
    ss += native_word("a");
    ss += native_word("sentence,");
    ss += native_word("with");
    ss += native_word("punctuation.");

    
    //BOOST_CHECK_EQUAL_COLLECTIONS(ss.begin(),ss.end(),tok.begin(),tok.end());
    
    fat_sentence ss2 = native_sentence("this: is a sentence, with punctuation.");
    
    BOOST_CHECK_EQUAL(ss, ss2);
    
    fat_sentence fs = foreign_sentence("MEINE KLEINE SCHWARZE KATZE");
    
    fat_sentence fs2(foreign_token);
    fs2 += foreign_word("MEINE");
    fs2 += foreign_word("KLEINE");
    fs2 += foreign_word("SCHWARZE");
    fs2 += foreign_word("KATZE");
    
    BOOST_CHECK_EQUAL(fs,fs2);
}
