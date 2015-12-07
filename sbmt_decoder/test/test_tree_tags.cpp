# include <sbmt/grammar/tree_tags.hpp>
# include <sbmt/grammar/lm_string.hpp>

# include <boost/test/auto_unit_test.hpp>
# include <algorithm>

void check_lisp_tree_to_tags (std::string const& tree, std::string const& tags)
{
    using namespace sbmt;
    using namespace std;
    
    fat_token_factory dict;
    BOOST_CHECKPOINT("create tree-tags");
    vector<fat_token> v = lisp_tree_tags(tree,dict);
    //copy(v.begin(),v.end(),ostream_iterator<fat_token>(cerr,"\" \""));
    BOOST_CHECKPOINT("create lm string");
    fat_lm_string s(tags,dict);
    BOOST_CHECKPOINT("feed into vector");
    vector<fat_token> w;
    for (fat_lm_string::iterator i = s.begin(); i != s.end(); ++i) {
        w.push_back(i->get_token());
    }
    BOOST_CHECKPOINT("check equal tree-tags");
    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), w.begin(), w.end());
    BOOST_CHECKPOINT("close tree-tag check");
}

BOOST_AUTO_TEST_CASE(test_tree_tags)
{
    check_lisp_tree_to_tags(
        "(TOP (S (NP the cat) (VP bites back) ) )"
      , "(TOP) (S) (NP) \"the\" \"cat\" (/NP) "
        "(VP) \"bites\" \"back\" (/VP) (/S) (/TOP)"
    );
    
}

BOOST_AUTO_TEST_CASE(test_tree_tags_paren_as_terminal)
{
    check_lisp_tree_to_tags(
        "(TOP (-LRB- () this is stupid (-RRB- )) )"
      , "(TOP) (-LRB-) \"(\" (/-LRB-) \"this\" \"is\" \"stupid\" (-RRB-) \")\" (/-RRB-) (/TOP)"
    );
}

BOOST_AUTO_TEST_CASE(test_tree_tags_paren_as_terminal2)
{
    check_lisp_tree_to_tags(
    "(TOP (S (PART (-LRB- () (A a) (SENT sentence) (W with) (PAREN parens) ) (-RRB- )) (STOP .) ) )"
    ,"(TOP) (S) (PART) (-LRB-) \"(\" (/-LRB-) (A) \"a\" (/A) (SENT) \"sentence\" (/SENT) (W) \"with\" (/W) "
     "(PAREN) \"parens\" (/PAREN) (/PART) (-RRB-) \")\" (/-RRB-) (STOP) \".\" (/STOP) (/S) (/TOP)");
}
