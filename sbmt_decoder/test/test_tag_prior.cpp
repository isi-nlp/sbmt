#if !defined (_MSC_VER)
#include "test_util.hpp"
//#define DEBUG_TAG_PRIOR
#include <sbmt/grammar/tag_prior.hpp>
#include "grammar_examples.hpp"


static char const *simple_grammar_nts=
    "NPB 5   NP-C 2   PP 0   VP 2.5   NP .5";


BOOST_AUTO_TEST_CASE(test_tag_prior)
{
    using namespace sbmt;
    using namespace std;

    grammar_in_mem gram1;

    std::istringstream i(grammar_nts);
    in_memory_dictionary &dict=gram1.dict();
    tag_prior p(string(grammar_nts),dict);
    score_t s_floor=.01;
    score_t u_floor=.1;
    tag_prior s(string(simple_grammar_nts),dict,s_floor,2);
    tag_prior u(string(simple_grammar_nts),dict,u_floor,0);

//    cerr << "\n\nnormalized, floor=.1,add=2:\n" << grammar_nts << "\n";p.print(cerr,dict);
    cerr << "\n\nnormalized, floor="<<s_floor<<",add=2:\n" << simple_grammar_nts << "\n";
    s.print(cerr,dict);
    cerr << "\n\nnormalized, floor="<<u_floor<<",add=0:\n"<< simple_grammar_nts << "\n";
    u.print(cerr,dict);
    cerr<<"\n\n";

    grammar_in_mem::rule_range r1 = gram1.all_rules();
    grammar_in_mem::rule_iterator ritr1 = r1.begin();

    indexed_token NPB=dict.tag("NPB");
    indexed_token NP=dict.tag("NP");
    indexed_token VP=dict.tag("VP");
    indexed_token PP=dict.tag("PP");
    indexed_token QP=dict.tag("QP");

    score_t tolerance=.001;
    LOGNUMBER_CHECK_EQ(s[NPB],.35);
    LOGNUMBER_CHECK_EQ(u[NPB],.5);
    LOGNUMBER_CHECK_EQ(s[QP],s_floor);
    LOGNUMBER_CHECK_EQ(u[QP],u_floor);
    LOGNUMBER_CHECK_EQ(s[PP],.1);
    LOGNUMBER_CHECK_EQ(u[PP],u_floor);

    init_grammar_marcu_staggard_wts(gram1);
}

#endif // if !defined(_MSC_VER)
