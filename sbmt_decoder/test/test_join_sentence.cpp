/*# include <sbmt/edge/sentence_info.hpp>
# include <sbmt/hash/substring.hpp>
# include <sbmt/hash/substring_hash.hpp>
# include <sbmt/search/force_sentence_filter.hpp>

# include <boost/test/auto_unit_test.hpp>

# include <string>
# include <iterator>
# include <algorithm>


// tests related to the state objects involved in force-decoding
// you will find span-based state objects and sentence based state objects
// span-based allows much faster force-decoding


using namespace sbmt;
using namespace std;

void test_gap( string span_str
             , std::pair<int,int> result)
{
    span_string sstr(span_str);
    std::pair<int,int> r = gap(span_str);
    stringstream err;
    err << "test_gap: gap(\"" << span_str << "\") == "
        << "(" << r.first << "," << r.second << ") != " 
        << "(" << result.first << "," << result.second << ")";
    BOOST_CHECK_MESSAGE(r == result, err.str()); 
}

template <class SITR>
void test_lm_to_span( string sentence
                    , string lmstring
                    , SITR spanstr_itr
                    , SITR spanstr_end )
{
    in_memory_dictionary dict;
    
    indexed_sentence str = native_sentence(sentence, dict);
    
    substring_hash_match<indexed_token> match(str.begin(),str.end());
    
    vector<span_string> span_str_vec;
    for(; spanstr_itr != spanstr_end; ++spanstr_itr) 
        span_str_vec.push_back(span_string(*spanstr_itr));
    
    indexed_lm_string lmstr(lmstring,dict);
    
    vector<span_string> span_str_out;
    span_strings_from_lm_string(lmstr, match, back_inserter(span_str_out));
    
    BOOST_CHECK_EQUAL_COLLECTIONS( span_str_vec.begin(), span_str_vec.end()
                                 , span_str_out.begin(), span_str_out.end() );
}

BOOST_AUTO_TEST_CASE(testing_gap)
{
    test_gap("x0 x1", make_pair(0,1));
    test_gap("x1 x0", make_pair(0,-1));
    test_gap("x0 [4,7] x1 [8,10]", make_pair(3,1));
    test_gap("x0 [4,7]", make_pair(3,0));
}


BOOST_AUTO_TEST_CASE(testing_sig)
{
    BOOST_CHECK(span_sig(0,10,span_sig::open_right)(span_t(0,5)) == true);
    BOOST_CHECK(span_sig(0,10,span_sig::open_right)(span_t(1,5)) == false);
    BOOST_CHECK(span_sig(0,10,span_sig::open_right)(span_t(0,11)) == false);
    
    BOOST_CHECK(span_sig(5,9,span_sig::open_left)(span_t(6,9)) == true);
    BOOST_CHECK(span_sig(5,9,span_sig::open_left)(span_t(5,9)) == true);
    BOOST_CHECK(span_sig(5,9,span_sig::open_left)(span_t(4,9)) == false);
    BOOST_CHECK(span_sig(5,9,span_sig::open_left)(span_t(5,8)) == false);
    BOOST_CHECK(span_sig(5,9,span_sig::open_left)(span_t(5,10)) == false);
    
    BOOST_CHECK(span_sig(5,9,span_sig::open_both)(span_t(6,8)) == true);
    BOOST_CHECK(span_sig(5,9,span_sig::open_both)(span_t(5,9)) == true);
    BOOST_CHECK(span_sig(5,9,span_sig::open_both)(span_t(6,10)) == false);
    BOOST_CHECK(span_sig(5,9,span_sig::open_both)(span_t(4,8)) == false);
    
    BOOST_CHECK_EQUAL( 
        signature(span_string("x0 [4,6] x1 [7,9]"),0)
      , span_sig(0,4,span_sig::open_left) 
    );
                     
    BOOST_CHECK_EQUAL( 
        signature(span_string("x0 [4,6] x1 [7,9]"),1)
      , span_sig(6,7,span_sig::open_none) 
    );
    
    BOOST_CHECK_EQUAL( 
        signature(span_string("x0 [4,6] x1"),1)
      , span_sig(6,USHRT_MAX,span_sig::open_right) 
    );
    
    BOOST_CHECK_EQUAL( 
        signature(span_string("x0 [4,6] x1"),0)
      , span_sig(0,4,span_sig::open_left) 
    );
    
    BOOST_CHECK_EQUAL( 
        signature(span_string("x0 x1"),0)
      , span_sig(0,USHRT_MAX,span_sig::open_both) 
    );
    
    BOOST_CHECK_EQUAL( 
        signature(span_string("x0 x1"),1)
      , span_sig(0,USHRT_MAX,span_sig::open_both) 
    );
    
    BOOST_CHECK_EQUAL(
        signature(span_string("[2,4] x0 x1 x2 [9,10]"),0)
      , span_sig(4,9,span_sig::open_right)
    );
    
    BOOST_CHECK_EQUAL(
        signature(span_string("[2,4] x0 x1 x2 [9,10]"),1)
      , span_sig(4,9,span_sig::open_both)
    );
    
    BOOST_CHECK_EQUAL(
        signature(span_string("[2,4] x0 x1 x2 [9,10]"),2)
      , span_sig(4,9,span_sig::open_left)
    );
    
}


BOOST_AUTO_TEST_CASE(test_lm_string_to_span_string)
{   
    string span_str1[3] = { "[0,1] x0 [1,2]"
                          , "[0,1] x0 [3,4]"
                          , "[2,3] x0 [3,4]" };
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , "\"abra\" x0 \"cad\""
                   , span_str1, span_str1 + 3 );
    
    string span_str2[2] = { "x2 [0,2] x1 x0", "x2 [2,4] x1 x0" };
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , "x2 \"abra\" \"cad\" x1 x0"
                   , span_str2, span_str2 + 2 );
                   
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , "\"abra\" x0 \"cad\""
                   , span_str1, span_str1 + 3 );

    string span_str3[2] = { "[0,2] x0", "[2,4] x0" };
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , "\"abra\" \"cad\" x0"
                   , span_str3, span_str3 + 2 );
                   
    string span_str3_5[2] = { "x0 [0,2]", "x0 [2,4]"};
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , "x0 \"abra\" \"cad\""
                   , span_str3_5, span_str3_5 + 2);
                   
    string span_str4[2] = { "[0,2] x0 [4,6] x1", "[2,4] x0 [4,6] x1" };
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , "\"abra\" \"cad\" x0 \"abra\" \"bad\" x1"
                   , span_str4, span_str4 + 2 );
                   
    
    string span_str5[3] = { "x1 x2 [0,1] x0 x3 [1,2] x4 x5"
                          , "x1 x2 [0,1] x0 x3 [3,4] x4 x5"
                          , "x1 x2 [2,3] x0 x3 [3,4] x4 x5" };
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , "x1 x2 \"abra\" x0 x3 \"cad\"  x4 x5"
                   , span_str5, span_str5 + 3 );
    
    string span_str6[13] = { "[0,2] x0 [3,5]"
                           , "[0,2] x0 [7,9]"
                           , "[0,2] x0 [12,14]"
                           , "[0,2] x0 [13,15]"
                           , "[1,3] x0 [3,5]"
                           , "[1,3] x0 [7,9]"
                           , "[1,3] x0 [12,14]"
                           , "[1,3] x0 [13,15]"
                           , "[5,7] x0 [7,9]"
                           , "[5,7] x0 [12,14]"
                           , "[5,7] x0 [13,15]"
                           , "[10,12] x0 [12,14]"
                           , "[10,12] x0 [13,15]" };
    ///               0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
    test_lm_to_span( "a a a b b a a b b c a a b b b c c"
                   , "\"a\" \"a\" x0 \"b\" \"b\""
                   , span_str6, span_str6 + 13 );
    
    string span_str7[31] = { "[0,2] x0 [3,5] x1 [9,10]"
                           , "[0,2] x0 [3,5] x1 [15,16]"
                           , "[0,2] x0 [3,5] x1 [16,17]"
                           , "[0,2] x0 [7,9] x1 [9,10]"
                           , "[0,2] x0 [7,9] x1 [15,16]"
                           , "[0,2] x0 [7,9] x1 [16,17]"
                           , "[0,2] x0 [12,14] x1 [15,16]"
                           , "[0,2] x0 [12,14] x1 [16,17]"
                           , "[0,2] x0 [13,15] x1 [15,16]"
                           , "[0,2] x0 [13,15] x1 [16,17]"
                           , "[1,3] x0 [3,5] x1 [9,10]"
                           , "[1,3] x0 [3,5] x1 [15,16]"
                           , "[1,3] x0 [3,5] x1 [16,17]"
                           , "[1,3] x0 [7,9] x1 [9,10]"
                           , "[1,3] x0 [7,9] x1 [15,16]"
                           , "[1,3] x0 [7,9] x1 [16,17]"
                           , "[1,3] x0 [12,14] x1 [15,16]"
                           , "[1,3] x0 [12,14] x1 [16,17]"
                           , "[1,3] x0 [13,15] x1 [15,16]"
                           , "[1,3] x0 [13,15] x1 [16,17]"
                           , "[5,7] x0 [7,9] x1 [9,10]"
                           , "[5,7] x0 [7,9] x1 [15,16]"
                           , "[5,7] x0 [7,9] x1 [16,17]"
                           , "[5,7] x0 [12,14] x1 [15,16]"
                           , "[5,7] x0 [12,14] x1 [16,17]"
                           , "[5,7] x0 [13,15] x1 [15,16]"
                           , "[5,7] x0 [13,15] x1 [16,17]"
                           , "[10,12] x0 [12,14] x1 [15,16]"
                           , "[10,12] x0 [12,14] x1 [16,17]"
                           , "[10,12] x0 [13,15] x1 [15,16]"
                           , "[10,12] x0 [13,15] x1 [16,17]" };
    ///               0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
    test_lm_to_span( "a a a b b a a b b c a a b b b c c"
                   , "\"a\" \"a\" x0 \"b\" \"b\" x1 \"c\""
                   , span_str7, span_str7 + 31 );
                   
    string span_str8[1] = { "x1 x0 x2" };
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , "x1 x0 x2"
                   , span_str8, span_str8 + 1 );

    string span_str9[1] = { "" };
    test_lm_to_span( "abra cad abra cad abra bad abra"
                   , ""
                   , span_str9, span_str9 + 1 );
    
}

void test_unary(string sstr, span_t arg, span_t result)
{
    stringstream ss;
    ss << "test_unary: " << sstr << " : " << arg << " : " << result;
    BOOST_CHECKPOINT(ss.str());

    span_string spanstr(sstr);
    
    std::pair<span_t,bool> r = join_spans(spanstr,arg);
    
    BOOST_CHECK(r.second == true);
    stringstream err;
    err << "join_spans(" << sstr << "," << arg << ") == "
        << r.first << " != " << result; 
    
    BOOST_CHECK_MESSAGE(r.first == result, err.str());
    
    BOOST_CHECKPOINT(ss.str());
}

void test_binary(string sstr, span_t arg1, span_t arg2, span_t result)
{
    stringstream ss;
    ss << "test_binary: " << sstr 
                          << " : " << arg1 
                          << " : " << arg2
                          << " : " << result;
    BOOST_CHECKPOINT(ss.str());

    span_string spanstr(sstr);
    
    std::pair<span_t,bool> r = join_spans(spanstr,arg1,arg2);
    
    BOOST_CHECK(r.second == true);
    stringstream err;
    err << "join_spans(" << sstr << "," << arg1 << "," << arg2 << ") == "
        << r.first << " != " << result; 
    
    BOOST_CHECK_MESSAGE(r.first == result, err.str());
    
    BOOST_CHECKPOINT(ss.str()); 
}

void test_binary( string lmstring
                , string first
                , string second 
                , string result )
{
    BOOST_CHECKPOINT( "test_binary begin : "+lmstring+
                                       " : "+first+
                                       " : "+second+
                                       " : "+result );
    in_memory_dictionary dict;
    indexed_lm_string lmstr(lmstring, dict);
    indexed_sentence s1 = native_sentence(first, dict) ,
                     s2 = native_sentence(second, dict) ,
                     r  = native_sentence(result, dict);
                     
    BOOST_CHECK(join_sentence(lmstr,s1,s2) == r);
    
    BOOST_CHECKPOINT("test_binary::lazy : "+lmstring+
                                      " : "+first+
                                      " : "+second+
                                      " : "+result );
    
    std::pair<lazy_join_sentence_iterator,lazy_join_sentence_iterator>
       lz = lazy_join_sentence(lmstr,s1,s2);
       
    BOOST_CHECK_EQUAL_COLLECTIONS(lz.first, lz.second, r.begin(), r.end());
    
    BOOST_CHECKPOINT( "test_binary end : "+lmstring+
                                     " : "+first+
                                     " : "+second+
                                     " : "+result );
}

void test_unary( string lmstring
               , string arg
               , string result )
{
    BOOST_CHECKPOINT("test_unary begin : "+lmstring+" : "+arg+" : "+result);
    in_memory_dictionary dict;
    indexed_lm_string lmstr(lmstring, dict);
    indexed_sentence s = native_sentence(arg, dict) ,
                     r = native_sentence(result, dict);
    
    BOOST_CHECK(join_sentence(lmstr,s) == r);
    
    BOOST_CHECKPOINT("test_unary::lazy : "+lmstring+" : "+arg+" : "+result);
    
    std::pair<lazy_join_sentence_iterator,lazy_join_sentence_iterator>
       lz = lazy_join_sentence(lmstr,s);
       
    BOOST_CHECK_EQUAL_COLLECTIONS(lz.first, lz.second, r.begin(), r.end());
    
    BOOST_CHECKPOINT("test_unary end : "+lmstring+" : "+arg+" : "+result);
}

BOOST_AUTO_TEST_CASE(test_join_sentence)
{   
    test_binary ( "\"foo\" x1 \"bar\" x0 \"bif\""
                , "a b"
                , "c d" 
                , "foo c d bar a b bif" );
    
    test_binary ( "[0,1] x1 [3,4] x0 [5,6]"
                , span_t(4,5)
                , span_t(1,3)
                , span_t(0,6) );
                
    test_unary ( ""
               , span_t(4,4)
               , span_t(0,0) );
    
    test_binary ( "\"foo\" x1 x0 \"bif\""
                , "a b"
                , "c d"
                , "foo c d a b bif" );
    
    test_binary ( "[4,6] x1 x0 [11,20]"
                , span_t(8,11)
                , span_t(6,8)
                , span_t(4,20) );
    
    test_binary( "x1 x0 \"bif\""
               , ""
               , "a b"
               , "a b bif" );
    
    test_binary( "x1 x0 [9,11]"
               , span_t(20,20)
               , span_t(4,9)
               , span_t(4,11) );
    
    test_binary ( "x1 x0 \"bif\""
                , "a b"
                , "" 
                , "a b bif" );
    
    test_binary ( "x1 x0 [5,10]"
                , span_t(1,5)
                , span_t(4,4)
                , span_t(1,10) );
    
    test_binary( "x0 x1 \"bif\""
               , ""
               , "a b"
               , "a b bif" );
               
    test_binary( "x0 x1 [6,8]"
               , span_t(0,0)
               , span_t(5,6)
               , span_t(5,8) );
    
    test_binary ( "x0 x1 \"bif\""
                , "a b"
                , "" 
                , "a b bif" );
                
    test_binary ( "0 1 [6,8]"
                , span_t(5,6)
                , span_t(0,0)
                , span_t(5,8) );
    
    test_binary( "\"bif\" x1 x0"
               , "c"
               , "a b"
               , "bif a b c" );
               
    test_binary( "[4,5] 1 0"
               , span_t(6,7)
               , span_t(5,6)
               , span_t(4,7) );
    
    test_binary( "\"bif\" x1 x0"
               , ""
               , "a b"
               , "bif a b" );
               
    test_binary( "[4,5] x1 x0"
               , span_t(20,20)
               , span_t(5,7)
               , span_t(4,7) );
    
    test_binary ( "\"bif\" x1 x0"
                , "a b"
                , "" 
                , "bif a b"  );
                
    test_binary( "[4,5] x1 x0"
               , span_t(20,20)
               , span_t(20,20)
               , span_t(4,5) );
    
    test_binary( "\"bif\" x0 x1"
               , ""
               , "a b"
               , "bif a b" );
    
    test_binary ( "\"bif\" x0 x1"
                , "a b"
                , "" 
                , "bif a b" );
                
    test_binary ( "[3,4] x0 x1"
                , span_t(4,6)
                , span_t(9,9)
                , span_t(3,6) );
    
    test_binary ( "\"bif\" x0 x1"
                , "a b"
                , "c" 
                , "bif a b c" );
                
    test_binary ("[3,4] x0 x1"
                , span_t(4,5)
                , span_t(5,6)
                , span_t(3,6) );
    
    test_unary ( "\"foo\" \"bar\" x0 \"bif\""
               , "a"
               , "foo bar a bif" );
    
    test_unary ( "[0,5] x0 [7,9]"
               , span_t(5,7)
               , span_t(0,9) );
    
    test_unary ( "\"foo\" \"bar\" x0 \"bif\""
               , ""
               , "foo bar bif" );
               
    test_unary ( "[1,6] x0 [6,7]"
               , span_t(2,2)
               , span_t(1,7) );
               
    test_unary ( "[1,6] x0 [6,7]"
               , span_t(8,8)
               , span_t(1,7) );
               
    test_unary ( "[1,6] x0"
               , span_t(8,8)
               , span_t(1,6) );
               
    test_unary ( "x0 [1,6]"
               , span_t(0,0)
               , span_t(1,6) );
}

template <class MatcherT>
void test_match(MatcherT const& match, string key, bool result)
{
    stringstream sstr;
    string text(match.get_text().first, match.get_text().second);
    sstr << boolalpha;
    sstr << "match(\"" << text << "\")(\"" << key << "\") != " << result;
    
    BOOST_CHECK_MESSAGE( match(key.begin(),key.end()) == result
                       , sstr.str() );
}

void test_match( substring_hash_match<char> const& match
               , string key
               , bool result
               , string spans)
{
    stringstream sstr(spans);
    set<span_t> spanset;
    while (sstr) { span_t s; sstr >> s; if(sstr) spanset.insert(s); }
    vector<span_t> spanset2;
    BOOST_CHECK_EQUAL(match(key.begin()
                           ,key.end()
                           ,back_inserter(spanset2))
                     ,result);
    sort(spanset2.begin(),spanset2.end());
    BOOST_CHECK_EQUAL_COLLECTIONS( spanset.begin(),  spanset.end()
                                 , spanset2.begin(), spanset2.end());
    
}

BOOST_AUTO_TEST_CASE(test_substring_match)
{
    string str = "abracadabra";
    substring_match<char> match(str.begin(),str.end());
    substring_hash_match<char> hashmatch(str.begin(),str.end());
    
    test_match(match,"abra",true);
    test_match(match,"brad",false);
    test_match(match,"xyz",false);
    test_match(match,"aaa",false);
    test_match(match,"",true);
    
    test_match(hashmatch,"abra",true,"[0,4][7,11]");
    test_match(hashmatch,"brad",false);
    test_match(hashmatch,"xyz",false);
    test_match(hashmatch,"aaa",false);
    test_match(hashmatch,"",true,"[0,0]"); 
    
    str = "the speed of the growth of next year of the "
          "japanese economy is the possible slow @-@ down";
    match = substring_match<char>(str.begin(),str.end());
    hashmatch = substring_hash_match<char>(str.begin(),str.end());
    test_match(match,"japanese slow speed of the growth",false);
    test_match(hashmatch,"japanese slow speed of the growth",false);
    
    string::iterator itr = str.begin(), end = str.end();
    for (; itr != end; ++itr) {
        string::iterator jtr = itr;
        for (; jtr != end; ++jtr) {
            test_match(match,string(itr,jtr),true);
            //test_match(hashmatch,string(itr,jtr),true);
        }
        test_match(match,string(itr,end),true);
        //test_match(hashmatch,string(itr,end),true);
    }
}
*/