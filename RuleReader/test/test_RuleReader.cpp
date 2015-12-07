#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>
#include <RuleReader/Rule.h>
#include <RuleReader/RuleNode.h>
#include <iostream>

using namespace std;
using namespace ns_RuleReader;

template <class ItrT> 
void print_map_t (ItrT begin, ItrT end) 
{
    for (ItrT itr = begin; itr != end; ++itr)
        std::cout <<"("<<itr->first<<","<<itr->second<<")"<<std::endl;
}

template <class ItrT> 
void print_coll_t (ItrT begin, ItrT end) 
{
    for (ItrT itr = begin; itr != end; ++itr)
        std::cout <<*itr<<std::endl;
}

BOOST_AUTO_TEST_CASE(test_RuleReader)
{   
    string rule_xrs = 
        "S(NP-C(x0:NPB) VP(MD(\"will\") VP-C(VB(\"be\") x1:VP-C)) .(\".\")) -> "
        "x0 \"A\" \"B\" x1 \".\" ### id=1"; 
    
    Rule r(rule_xrs);
    
    BOOST_CHECK_EQUAL(12,r.numNodes());
    /// attributes functionality
    BOOST_CHECK(r.existsAttribute("id"));
    BOOST_CHECK_EQUAL("1",r.getAttributeValue("id"));
    BOOST_CHECK(!r.existsAttribute("foo"));
    
    BOOST_CHECK(!r.is_binarized_rule());
    
    char* rhs_states_vec[5] = {"x0","","","x1",""};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSStates()->begin(), r.getRHSStates()->end(),
        rhs_states_vec,            rhs_states_vec + 5
    );
    
    BOOST_CHECK_EQUAL((*r.getStateLabels())["x0"],"NPB");
    BOOST_CHECK_EQUAL((*r.getStateLabels())["x1"],"VP-C");
    
    char* rhs_lexical_items_vec[5] = {"","A","B","","."};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSLexicalItems()->begin(), r.getRHSLexicalItems()->end(),
        rhs_lexical_items_vec,           rhs_lexical_items_vec + 5
    );
    
    char* rhs_constituents_vec[5] = {"","","","",""};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSConstituents()->begin(), r.getRHSConstituents()->end(),
        rhs_constituents_vec,            rhs_constituents_vec + 5
    );
    
}

BOOST_AUTO_TEST_CASE(test_RuleReader_brf1)
{   
    string rule_xrs = 
        "VP:VL: VP(DT(\"a\") NP(x0:CC DT(\"b\") DT(\"c\") x1:NN VB(\"d\")))"
        " -> V[V[NN_\"A\"]_\"B\"] V[CC_\"C\"]"
        " ### min-top-k=256 id=1 virtual_label=no complete_subtree=yes "
        "lm_string={{{\"a\" 1 \"b\" \"c\" 0 \"d\"}}} sblm_string={{{1 0 }}} "
        "lm=yes sblm=yes rule_file_line_number=1";
    
    Rule r(rule_xrs);
    
    /// attributes functionality
    BOOST_CHECK(r.existsAttribute("rule_file_line_number"));
    BOOST_CHECK_EQUAL("1",r.getAttributeValue("rule_file_line_number"));
    BOOST_CHECK(!r.existsAttribute("foo"));
    
    BOOST_CHECK(r.is_binarized_rule());
    BOOST_CHECK_EQUAL(r.get_label(),"VP");
    BOOST_CHECK(!r.is_virtual_label());
    
    char* rhs_states_vec[2] = {"",""};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSStates()->begin(), r.getRHSStates()->end(),
        rhs_states_vec,            rhs_states_vec + 2
    );
    
    char* rhs_lexical_items_vec[2] = {"",""};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSLexicalItems()->begin(), r.getRHSLexicalItems()->end(),
        rhs_lexical_items_vec,           rhs_lexical_items_vec + 2
    );
    
    char* rhs_constituents_vec[2] = {"V[V[NN_\"A\"]_\"B\"]","V[CC_\"C\"]"};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSConstituents()->begin(), r.getRHSConstituents()->end(),
        rhs_constituents_vec,            rhs_constituents_vec + 2
    );
}

BOOST_AUTO_TEST_CASE(test_RuleReader_brf2)
{    
    string rule_brf = 
        "V[CC_\"C\"]:VL: V[CC_\"C\"](x0:CC) -> x0 \"C\" " 
        "### complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
        "sblm_string={{{}}} virtual_label=yes rule_file_line_number={{{1}}}";
    
    Rule r(rule_brf);
    
    //print_map_t(r.getAttributes()->begin(),r.getAttributes()->end());
    
    /// attributes functionality
    BOOST_CHECK(r.existsAttribute("rule_file_line_number"));
    BOOST_CHECK_EQUAL("1",r.getAttributeValue("rule_file_line_number"));
    BOOST_CHECK(!r.existsAttribute("foo"));
    
    BOOST_CHECK_EQUAL(r.get_label(),"V[CC_\"C\"]");
    
    BOOST_CHECK(r.is_binarized_rule());
    BOOST_CHECK(r.is_virtual_label());
    
    char* rhs_states_vec[2] = {"x0",""};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSStates()->begin(), r.getRHSStates()->end(),
        rhs_states_vec,            rhs_states_vec + 2
    );
    
    BOOST_CHECK_EQUAL((*r.getStateLabels())["x0"],"CC");
    
    char* rhs_lexical_items_vec[2] = {"","C"};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSLexicalItems()->begin(), r.getRHSLexicalItems()->end(),
        rhs_lexical_items_vec,           rhs_lexical_items_vec + 2
    );
    
    char* rhs_constituents_vec[2] = {"",""};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        r.getRHSConstituents()->begin(), r.getRHSConstituents()->end(),
        rhs_constituents_vec,            rhs_constituents_vec + 2
    );
}

BOOST_AUTO_TEST_CASE(test_RuleReader_RuleNode)
{
    string rule_xrs = 
        "S(NP-C(x0:NPB) VP(MD(\"will\") VP-C(VB(\"be\") x1:VP-C)) .(\".\")) -> "
        "x0 \"A\" \"B\" x1 \".\" ### id=1"; 
    
    Rule r(rule_xrs);
    RuleNode* current = r.getLHSRoot();
    
    BOOST_CHECK_EQUAL("S",current->getString(false,false));
    
    BOOST_CHECK_EQUAL(current->getChildren()->size(), 3);

    char* children_of_S[3] = {"NP-C", "VP", "."};
    for (int x = 0; x != 3; ++x) {
        BOOST_CHECK_EQUAL(children_of_S[x], current->getChildren()->at(x)->getString(false,false));
    }
    
    current = current->getChildren()->at(2);
    
    BOOST_CHECK_EQUAL(current->getChildren()->size(), 1);
    BOOST_CHECK_EQUAL(".",current->getChildren()->at(0)->getString(false,false));
    BOOST_CHECK(current->isNonTerminal());
    BOOST_CHECK(current->isPreTerminal());
}
