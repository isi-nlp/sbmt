#include <boost/test/auto_unit_test.hpp>
#include <boost/ref.hpp>
#include <sbmt/logmath.hpp>
#include <sbmt/grammar/brf_file_reader.hpp>
#include <map>
#include <set>

#include "test_util.hpp"

using namespace sbmt;
using namespace sbmt::logmath;
using namespace ns_RuleReader;
using namespace std;

namespace std {
template <class T1, class T2>
std::ostream& operator << (std::ostream& os, std::pair<T1,T2> const& p)
{
    return os << "(" << p.first << "," << p.second << ")";
}
} // namespace std

////////////////////////////////////////////////////////////////////////////////
/*
BOOST_AUTO_TEST_CASE(test_score_extraction)
{
    std::string brf_rule = 
        "VP:VL: VP(DT(\"a\") NP(x0:CC DT(\"b\") DT(\"c\") x1:NN VB(\"d\"))) -> "
        "V[V[\"A\"_NN]_\"B\"] V[CC_\"C\"] ### foos=0.01 binprob=e^-2 id=1 "
        "virtual_label=no complete_subtree=yes "
        "lm_string={{{\"a\" 1 \"b\" \"c\" 0 \"d\"}}} sblm_string={{{1 0 }}} "
        "lm=yes sblm=yes rule_file_line_number=1 "
        "rhs={{{\"A\" x1 \"B\" x0 \"C\"}}}";
        
    std::string xrs_rule = 
        "VP(DT(\"a\") NP(x0:CC DT(\"b\") DT(\"c\") x1:NN VB(\"d\"))) -> "
        "\"A\" x1 \"B\" x0 \"C\" ### id=1";
    Rule r(brf_rule);
    
    std::string xrs_rule2 = 
        "VP(DT(\"a\") NP(x0:CC DT(\"b\") DT(\"c\") x1:NN VB(\"d\"))) -> "
        "\"A\" x1 \"B\" x0 \"D\" ### id=1";
    
    
    fat_token_factory tf;
    syntax_rule<fat_token> syn1 = sbmt::detail::create_syntax_rule(r,tf);
    syntax_rule<fat_token> syn2(xrs_rule,tf);
    syntax_rule<fat_token> syn3(xrs_rule2,tf);
    
    BOOST_CHECK_EQUAL(to_string(syn1,tf),to_string(syn2,tf));
    BOOST_CHECK(to_string(syn1,tf) != to_string(syn3,tf));
    
    pair<string const, score_t>
    scores[] = { make_pair("foos",    lexical_cast<score_t>("0.01"))
               , make_pair("binprob", lexical_cast<score_t>("e^-2")) };
                       
    map<string, score_t> score_set(scores, scores + 2);
    map<string, score_t> score_set2;
    
    sbmt::detail::extract_scores(r, score_set2);
    
    BOOST_CHECK_EQUAL_COLLECTIONS( score_set.begin(),  score_set.end()
                                 , score_set2.begin(), score_set2.end() );
}
*/

////////////////////////////////////////////////////////////////////////////////

struct xrs_cb 
{
    void operator()( syntax_rule<fat_token> const& syn
                   , map<string,score_t> const& scores )
    {
        rule_set.insert(to_string(syn,fat_token_factory()));
        score_map.insert(make_pair(syn.id(),scores));
    }
    set< string > rule_set;
    map< size_t, map<string,score_t> > score_map;
};

struct brf_pair
{
    brf_pair(fat_binary_rule const& brf, vector<long> const& rule_ids)
    : brf(brf), rule_ids(rule_ids) {}
    fat_binary_rule brf;
    vector<long>  rule_ids;
};

std::ostream& operator << (std::ostream& os, brf_pair const& brf_pair)
{
    os << brf_pair.brf << " id={{{" << brf_pair.rule_ids[0];
    for (unsigned i = 1; i < brf_pair.rule_ids.size(); ++i)
        os << " " << brf_pair.rule_ids[i];
    os << "}}}";
    return os;
}

bool operator == (brf_pair const& b1, brf_pair const& b2) 
{
    return b1.brf == b2.brf and b1.rule_ids == b2.rule_ids;
}

bool operator != (brf_pair const& b1, brf_pair const& b2) 
{
    return !(b1 == b2);
}

struct brf_cb
{
    void operator()(fat_binary_rule const& brf, vector<long> const& rule_ids)
    {
        binary_rules.push_back(brf_pair(brf,rule_ids));
    }
    
    vector<brf_pair> binary_rules;
};
/*
BOOST_AUTO_TEST_CASE(test_rule_read)
{
    stringstream rule_str;
    
    string str1="VP:VL: VP(DT(\"a\") NP(x0:CC DT(\"b\") DT(\"c\") x1:NN VB(\"d\"))) -> "
                "V[V[\"A\"_NN]_\"B\"] V[CC_\"C\"] ### foos=0.1 binprob=e^-2 id=1 "
                "virtual_label=no complete_subtree=yes "
                "lm_string={{{\"a\" 1 \"b\" \"c\" 0 \"d\"}}} "
                "sblm_string={{{1 0 }}} lm=yes sblm=yes rule_file_line_number=1 "
                "rhs={{{\"A\" x1 \"B\" x0 \"C\"}}}";
                
    size_t idfor1[] = {1};
                
    string str2="NP:VL: NP(x0:CC DT(\"b\") x1:NN VB(\"d\")) -> "
                "V[NN_\"B\"] V[CC_\"C\"] ### foos=0.8 binprob=e^-45 id=2 "
                "virtual_label=no complete_subtree=yes "
                "lm_string={{{1 \"b\" 0 \"d\"}}} sblm_string={{{1 0 }}} lm=yes "
                "sblm=yes rule_file_line_number=3 "
                "rhs={{{x1 \"B\" x0 \"C\"}}}";
    
    size_t idfor2[] = {2};
                
    string str3="XP:VL: XP(x0:CC DT(\"b\") x1:NN VB(\"d\")) -> "
                "V[CC_\"B\"] V[NN_\"C\"] ### foos=0.4 binprob=e^-22 id=3 "
                "virtual_label=no complete_subtree=yes "
                "lm_string={{{0 \"b\" 1 \"d\"}}} sblm_string={{{0 1 }}} lm=yes "
                "sblm=yes rule_file_line_number=5 "
                "rhs={{{x0 \"B\" x1 \"C\"}}}";
                
    size_t idfor3[] = {3};
                
    string str4="V[\"A\"_NN]:VL: V[\"A\"_NN](x0:NN) -> \"A\" x0 ### "
                "complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
                "sblm_string={{{}}} virtual_label=yes id={{{1}}}";
                
    size_t idfor4[] = {1};
                
    string str5="V[V[\"A\"_NN]_\"B\"]:VL: V[V[\"A\"_NN]_\"B\"](x0:V[\"A\"_NN]) -> "
                "x0 \"B\" ### complete_subtree=no sblm=no lm=yes "
                "lm_string={{{0}}} sblm_string={{{}}} virtual_label=yes "
                "id={{{1}}}";
                
    size_t idfor5[] = {1};
                
    string str6="V[CC_\"C\"]:VL: V[CC_\"C\"](x0:CC) -> x0 \"C\" ### "
                "complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
                "sblm_string={{{}}} virtual_label=yes id={{{1 2}}}";
                
    size_t idfor6[] = {1,2};
                
    string str7="V[NN_\"B\"]:VL: V[NN_\"B\"](x0:NN) -> x0 \"B\" ### "
                "complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
                "sblm_string={{{}}} virtual_label=yes id={{{2}}}";
   
    size_t idfor7[] = {2};
                
    string str8="V[CC_\"B\"]:VL: V[CC_\"B\"](x0:CC) -> x0 \"B\" ### "
                "complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
                "sblm_string={{{}}} virtual_label=yes id={{{3}}}";
                
    size_t idfor8[] = {3};
    
    string str9="V[NN_\"C\"]:VL: V[NN_\"C\"](x0:NN) -> x0 \"C\" ### "
                "complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
                "sblm_string={{{}}} virtual_label=yes id={{{3}}}";
                
    size_t idfor9[] = {3};
    
    rule_str << str1 << endl;
    rule_str << str2 << endl;
    rule_str << str3 << endl;
    rule_str << str4 << endl;
    rule_str << str5 << endl;
    rule_str << str6 << endl;
    rule_str << str7 << endl;
    rule_str << str8 << endl;
    rule_str << str9 << endl;
    
    string syn_str[] = 
    {  string("VP(DT(\"a\") NP(x3:CC DT(\"b\") DT(\"c\") x1:NN VB(\"d\"))) -> "
       "\"A\" x1 \"B\" x3 \"C\" ### id=1" )
     , string("NP(x2:CC DT(\"b\") x0:NN VB(\"d\")) -> "
       "x0 \"B\" x2 \"C\" ### id=2") 
     , string("XP(x0:CC DT(\"b\") x2:NN VB(\"d\")) -> "
       "x0 \"B\" x2 \"C\" ### id=3")
    };
    
    brf_pair brf_list[] =
    {
        brf_pair( fat_binary_rule(str1,fat_token_factory())
                , vector<long>(idfor1, idfor1 + 1))
      , brf_pair( fat_binary_rule(str2,fat_token_factory())
                , vector<long>(idfor2, idfor2 + 1))
      , brf_pair( fat_binary_rule(str3,fat_token_factory())
                , vector<long>(idfor3, idfor3 + 1))
      , brf_pair( fat_binary_rule(str4,fat_token_factory())
                , vector<long>(idfor4, idfor4 + 1))
      , brf_pair( fat_binary_rule(str5,fat_token_factory())
                , vector<long>(idfor5, idfor5 + 1))
      , brf_pair( fat_binary_rule(str6,fat_token_factory())
                , vector<long>(idfor6, idfor6 + 2))
      , brf_pair( fat_binary_rule(str7,fat_token_factory())
                , vector<long>(idfor7, idfor7 + 1))
      , brf_pair( fat_binary_rule(str8,fat_token_factory())
                , vector<long>(idfor8, idfor8 + 1))
      , brf_pair( fat_binary_rule(str9,fat_token_factory())
                , vector<long>(idfor9, idfor9 + 1))
    };
    
    set<string> syn_set(syn_str, syn_str + 3);
                
    brf_stream_reader reader(rule_str);
    xrs_cb cb;
    brf_cb bcb;
    fat_token_factory tf;
    reader.set_handlers(boost::ref(cb),boost::ref(bcb),tf);
    reader.read();
    
    BOOST_CHECK_EQUAL_COLLECTIONS( syn_set.begin(), syn_set.end() 
                                 , cb.rule_set.begin(), cb.rule_set.end() );
                                 
    BOOST_CHECK_EQUAL_COLLECTIONS( brf_list, brf_list + 9
                                 , bcb.binary_rules.begin(), bcb.binary_rules.end());
}
*/
