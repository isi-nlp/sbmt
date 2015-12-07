#include <boost/test/auto_unit_test.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <sbmt/grammar/lm_string.hpp>
#include <sbmt/grammar/rule_input.hpp>
#include <string>

using namespace std;
using namespace sbmt;

struct lm_scoreable_construct_
{
    template <class TF> 
    bool operator() (TF& tf, std::string const& str) const
    {
        if (str == "yes") return true;
        else if (str == "no") return false;
        else throw std::runtime_error("\"lm\" field must be \"yes\" or \"no\"");
    }
};

struct lm_string_construct_ {
    
    template <class TF>
    lm_string<typename TF::token_type> 
    operator()(TF& tf, std::string const& str) const
    {
        return lm_string<typename TF::token_type>(str,tf);
    }
};

BOOST_AUTO_TEST_CASE(test_rule_input)
{
    string lm_string_text = "\"a\" 1 \"b\" \"c\" 0 \"d\"";
    string brf_rule = 
        "VP:VL: VP(DT(\"a\") NP(x0:CC DT(\"b\") DT(\"c\") x1:NN VB(\"d\")))"
        " -> V[V[NN_\"A\"]_\"B\"] V[CC_\"C\"]"
        " ### min-top-k=256 id=1 virtual_label=no complete_subtree=yes "
        "lm_string={{{" + lm_string_text + "}}} sblm_string={{{1 0 }}} "
        "lm=yes sblm=yes rule_file_line_number=1";
    
    rule_topology<fat_token> rt( tag("VP")
                               , virtual_tag("V[V[NN_\"A\"]_\"B\"]")
                               , virtual_tag("V[CC_\"C\"]") );
    fat_token_factory tf;
    fat_lm_string lmstr = fat_lm_string(lm_string_text,tf);
    
    property_constructors<fat_token_factory> pc;
    size_t lm_id = pc.register_constructor("lm",lm_scoreable_construct_());
    size_t lm_string_id = pc.register_constructor("lm_string",lm_string_construct_());
    size_t dlm_string_id = pc.register_constructor("dlm_string",lm_string_construct_());
    
    fat_binary_rule ri(brf_rule, tf, pc);
    
    BOOST_CHECK_EQUAL(ri.topology(), rt);
    BOOST_CHECK(ri.rhs_size() == 2);
    BOOST_CHECK(rt.rhs_size() == 2);
    BOOST_CHECK_EQUAL(ri.lhs(), rt.lhs());
    BOOST_CHECK_EQUAL(ri.rhs(0), rt.rhs(0));
    BOOST_CHECK_EQUAL(ri.rhs(1), rt.rhs(1));
    
    BOOST_REQUIRE(ri.has_property(lm_id) == true);
    BOOST_REQUIRE(ri.has_property(lm_string_id) == true);
    BOOST_CHECK(ri.has_property(dlm_string_id) == false);
    BOOST_CHECK_EQUAL(ri.get_property<bool>(lm_id), true);
    BOOST_CHECK(ri.get_property<fat_lm_string>(lm_string_id) == lmstr);
    
    fat_binary_rule ri2 = ri;
    BOOST_CHECK_EQUAL(ri2,ri);
    
    brf_rule =
        "V[CC_\"C\"]:VL: V[CC_\"C\"](x0:CC) -> x0 \"C\" " 
        "### complete_subtree=no sblm=no lm=yes lm_string={{{0}}} "
        "sblm_string={{{}}} virtual_label=yes rule_file_line_number={{{1}}}";
        
    ri = fat_binary_rule(brf_rule,tf,pc);
    ri2 = ri;
    
    rt = rule_topology<fat_token>( virtual_tag("V[CC_\"C\"]")
                                 , tag("CC")
                                 , foreign_word("C") );
    
    BOOST_CHECK_EQUAL(ri2,ri);
    BOOST_CHECK_EQUAL(ri.topology(),rt);
    
    fat_binary_rule ri3 = ri2;
}
