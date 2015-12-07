#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <sbmt/token.hpp>

#include <boost/test/auto_unit_test.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/random.hpp>
#include <boost/tuple/tuple.hpp>
#include <set>
#include <sstream>

using namespace sbmt;
using namespace boost;

BOOST_AUTO_TEST_CASE(test_token_eq)
{
    in_memory_dictionary dict;
    indexed_token foo_native  = dict.native_word("foo");
    indexed_token foo_foreign = dict.foreign_word("foo");
    indexed_token bar_native  = dict.native_word("bar");
    indexed_token foo_native2 = dict.native_word("foo");
    
    BOOST_CHECK(foo_native  != bar_native);
    BOOST_CHECK(foo_native  != foo_foreign);
    BOOST_CHECK(foo_foreign != bar_native);
    BOOST_CHECK_EQUAL(foo_native,foo_native2);
}

////////////////////////////////////////////////////////////////////////////////
///
/// generates a random tok_storage object (a token without the caching)
/// consisting of a single character-length label
///
/// generates one of 26*3 + 1 symbols, with a one-in-four chance of generating
/// a "TOP"
///
////////////////////////////////////////////////////////////////////////////////
class random_token {
    mt19937 gen;
    char lbl_str[2];
    variate_generator<mt19937&, uniform_int<> > rand_type;
    variate_generator<mt19937&, uniform_int<> > rand_label;
public:
    random_token()
    : rand_type(gen,uniform_int<>(0,4))    // range of token::type_t enum
    , rand_label(gen,uniform_int<>(65,89)) // ANSI range of capital letters
    {
        lbl_str[1] = 0;
    }
    
    fat_token operator()()
    {
        token_type_id t = token_type_id(rand_type());
        lbl_str[0] = char(rand_label());
        return fat_token(lbl_str,t);
    } 
};

////////////////////////////////////////////////////////////////////////////////
///
///  i havent yet imported an autotest to look for where hash_set is defined, 
///  so for now i abuse multi-index container until i do.
///
////////////////////////////////////////////////////////////////////////////////
template <class T> class boost_hash_set
: public multi_index_container< 
            T
          , multi_index::indexed_by<
                multi_index::hashed_unique<
                    multi_index::identity<T> 
                >
            >
        >
{};

////////////////////////////////////////////////////////////////////////////////
///
///  tests that there is consistency between how token is hashed and compared
///  (just integer equality), with how tok_storage is hashed and compared.
///  goes back and forth attempting to add and remove a small collection of
///
////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_token_hash)
{
    typedef boost_hash_set<fat_token> fat_token_set_t;
    fat_token_set_t tok_storage_set;
    
    typedef boost_hash_set<indexed_token> token_set_t;
    token_set_t token_set;
    
    random_token rt;
    in_memory_dictionary dict;

    indexed_token frntk = dict.foreign_word("foo");
    BOOST_CHECK(is_lexical(frntk));
    BOOST_CHECK_EQUAL(frntk.type(),foreign_token);
    BOOST_CHECK_EQUAL(dict.label(frntk), "foo");
    
    indexed_token frntk2 = dict.create_token("fob",native_token);
    BOOST_CHECK(is_lexical(frntk2));
    BOOST_CHECK_EQUAL(frntk2.type(),native_token);
    BOOST_CHECK_EQUAL(dict.label(frntk2), "fob");
    
    indexed_token frntk3 = dict.foreign_word("foo");
    BOOST_CHECK(is_lexical(frntk3));
    BOOST_CHECK_EQUAL(frntk3.type(),foreign_token);
    BOOST_CHECK_EQUAL(dict.label(frntk3),"foo");
    
    indexed_token frntk4 = dict.create_token("fob",native_token);
    BOOST_CHECK(is_lexical(frntk4));
    BOOST_CHECK_EQUAL(frntk4.type(),native_token);
    BOOST_CHECK_EQUAL(dict.label(frntk4),"fob");
    
    indexed_token tagtk = dict.tag("foo");
    BOOST_CHECK(is_nonterminal(tagtk));
    BOOST_CHECK_EQUAL(tagtk.type(),tag_token);
    BOOST_CHECK_EQUAL(dict.label(tagtk), "foo");
    
    indexed_token tagtk2 = dict.create_token("fob",virtual_tag_token);
    BOOST_CHECK(is_nonterminal(tagtk2));
    BOOST_CHECK_EQUAL(tagtk2.type(),virtual_tag_token);
    BOOST_CHECK_EQUAL(dict.label(tagtk2), "fob");
    
    indexed_token tagtk3 = dict.tag("foo");
    BOOST_CHECK(is_nonterminal(tagtk3));
    BOOST_CHECK_EQUAL(tagtk3.type(),tag_token);
    BOOST_CHECK_EQUAL(dict.label(tagtk3),"foo");
    
    indexed_token tagtk4 = dict.create_token("fob",virtual_tag_token);
    BOOST_CHECK(is_nonterminal(tagtk4));
    BOOST_CHECK_EQUAL(tagtk4.type(),virtual_tag_token);
    BOOST_CHECK_EQUAL(dict.label(tagtk4),"fob");
    
    std::stringstream strstr;
    archive::text_oarchive oa(strstr);
    oa & dict;
    archive::text_iarchive ia(strstr);
    ia & dict;
    
    BOOST_CHECK_EQUAL(dict.label(frntk3),"foo");
    BOOST_CHECK_EQUAL(dict.label(frntk4),"fob");
    BOOST_CHECK_EQUAL(dict.label(tagtk2), "fob");
    BOOST_CHECK_EQUAL(dict.label(tagtk3),"foo");
    BOOST_CHECK_EQUAL(dict.label(tagtk4),"fob");
    
    
    for (int i = 0; i != 10000; ++i) {
        fat_token     tok1 = rt();
        indexed_token tok2 = dict.create_token(tok1.label(),tok1.type()); 
        std::stringstream str;
        BOOST_CHECK_EQUAL(tok1.type(),tok2.type());
        str << tok1 <<" type:"<< tok1.type()<<" idx:" << tok2 << " caused failure";
        if (i % 2 == 0) {           
            BOOST_CHECK_MESSAGE( 
                tok_storage_set.insert(tok1).second == 
                token_set.insert(tok2).second
              , str.str()
            );
        } else {
            BOOST_CHECK_MESSAGE(
                tok_storage_set.erase(tok1) == token_set.erase(tok2)
              , str.str()
            );
        }
    }
}



BOOST_AUTO_TEST_CASE(test_dictionary_iterator)
{
    using namespace std;
    in_memory_dictionary dict;
    set<string> native_words;
    
    dict.native_word("foo");
    dict.native_word("foo");
    dict.native_word("bar");
    dict.native_word("bif");
    dict.native_word("<s>");
    dict.native_word("</s>");
    dict.native_word("<unknown-word/>");
    dict.native_word("<separator/>");
    dict.native_word("<epsilon/>");
    
    native_words.insert("<unknown-word/>");
    native_words.insert("<separator/>");
    native_words.insert("<epsilon/>");
    native_words.insert("<s>");
    native_words.insert("</s>");
    native_words.insert("foo");
    native_words.insert("bar");
    native_words.insert("bif");
    
    in_memory_dictionary::range r = dict.native_words();
    set<string> from_dict;
    for (in_memory_dictionary::iterator i = r.begin(); i != r.end(); ++i) {
        from_dict.insert(dict.label(*i));
    }
    
    BOOST_CHECK_EQUAL_COLLECTIONS( from_dict.begin(), from_dict.end()
                                 , native_words.begin(), native_words.end() );
}
