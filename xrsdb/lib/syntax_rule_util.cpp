# include <syntax_rule_util.hpp>
# include <boost/regex.hpp>
# include <boost/tokenizer.hpp>
# include <sbmt/grammar/syntax_rule.hpp>
# include <boost/range.hpp>
# include <collapsed_signature_iterator.hpp>
# include "lattice_reader.hpp"

using namespace boost;
using namespace sbmt;
using namespace std;

namespace xrsdb {

namespace {
regex rhs(" -> (.*?) ### ");
}

vector<indexed_token>
from_string( string const& sss, indexed_token_factory& tf)
{
    //std::cout << "from_string: " << sss << std::endl;
    //std::cout << token_label(tf);
    using namespace xrsdb;
    smatch what;
    regex_search(sss,what,rhs);
    string str = what.str(1);
    boost::char_separator<char> sep(" ");
    boost::tokenizer<boost::char_separator<char> > toker(str,sep);
    vector<indexed_token> v;

    boost::tokenizer<boost::char_separator<char> >::iterator itr = toker.begin(),
                                                             end = toker.end();

    wildcard_array wc(tf);
    int wccount = 0;
    for (; itr != end; ++itr) {
        if ((*itr)[0] == '"') {
            string tok(itr->begin() + 1, itr->end() - 1);
            indexed_token t = tf.foreign_word(tok);
            if (wccount > 0) {
                v.push_back(wc[wccount]);
                //std::cout << wc[wccount] << ' ';
                wccount = 0;
            }
            v.push_back(t);
            //std::cout << t << ' ';
        } else {
            ++wccount;
        }
    }
    if (wccount > 0) {
        v.push_back(wc[wccount]);
        //std::cout << wc[wccount] << ' ';
        wccount = 0;
    }
    return v;
}

vector<indexed_token>
from_sig( string const& sss, indexed_token_factory& tf)
{
    //std::cout << "from_sig: " << sss << std::endl;
    //std::cout << token_label(tf);
    using namespace xrsdb;

    boost::char_separator<char> sep(" ");
    boost::tokenizer<boost::char_separator<char> > toker(sss,sep);
    vector<indexed_token> v;

    boost::tokenizer<boost::char_separator<char> >::iterator itr = toker.begin(),
                                                             end = toker.end();

    wildcard_array wc(tf);
    int wccount = 0;
    for (; itr != end; ++itr) {
        if ((*itr)[0] == '"') {
            string tok(itr->begin() + 1, itr->end() - 1);
            indexed_token t = tf.foreign_word(tok);
            if (wccount > 0) {
                v.push_back(wc[wccount]);
                //std::cout << wc[wccount] << ' ';
                wccount = 0;
            }
            v.push_back(t);
            //std::cout << t << ' ';
        } else {
            assert (wccount == 0);
            wccount = boost::lexical_cast<int>(*itr);
        }
    }
    if (wccount > 0) {
        v.push_back(wc[wccount]);
        //std::cout << wc[wccount] << ' ';
        wccount = 0;
    }
    return v;
}

template <class Out>
Out rule_to_string( indexed_syntax_rule const& rule
                  , indexed_syntax_rule::tree_node const& nd
                  , indexed_token_factory& dict
                  , indexed_token const& wc
                  , bool out_indices
                  , Out out )
{
    if (nd.indexed()) {
        if (out_indices) {
            *out = nd.get_token();
            ++out;
        }
        *out = wc;
        ++out;
        if (out_indices) {
            *out = dict.tag("/" + dict.label(nd.get_token()));
        }
    } else if (nd.lexical()) {
        *out = nd.get_token();
    } else {
        *out = nd.get_token();
        indexed_syntax_rule::lhs_children_iterator ci = nd.children_begin(),
                                                   ce = nd.children_end();
        for (; ci != ce; ++ci) {
            out = rule_to_string(rule,*ci,dict,wc,out_indices,out);
        }
        *out = dict.tag("/" + dict.label(nd.get_token()));
    }
    ++out;
    return out;
}

vector<indexed_token>
forcetree_sig(string const& sss, indexed_token_factory& tf)
{
    using xrsdb::collapse_signature;

    indexed_token wc = tf.virtual_tag("0");

    vector<indexed_token> v;
    try {
        indexed_syntax_rule rule(sss,tf);
        rule_to_string(rule,*rule.lhs_root(),tf,wc,true,back_inserter(v));
        v.push_back(wc);
        vector<indexed_token> source;
        indexed_syntax_rule::rhs_iterator ri = rule.rhs_begin(),
                                          re = rule.rhs_end();
        for (; ri != re; ++ri) {
            if (ri->indexed()) source.push_back(wc);
            else source.push_back(ri->get_token());
        }

        v.insert( v.end()
                , boost::begin(collapse_signature(source,wc))
                , boost::end(collapse_signature(source,wc))
                );
    } catch(...) {}
    return v;
}

vector<indexed_token>
stateful_forcetree_sig( string const& sss
                      , indexed_token_factory& tf )
{
    using xrsdb::collapse_signature;

    indexed_token wc = tf.virtual_tag("0");
    vector<indexed_token> v;
    try {
        indexed_syntax_rule rule(sss,tf);
        rule_to_string( rule
                      , *rule.lhs_root()->children_begin()
                      , tf
                      , wc
                      , false
                      , back_inserter(v) );
        v.push_back(wc);
        vector<indexed_token> source;
        indexed_syntax_rule::rhs_iterator ri = rule.rhs_begin(),
                                          re = rule.rhs_end();
        for (; ri != re; ++ri) {
            if (ri->indexed()) source.push_back(wc);
            else source.push_back(ri->get_token());
        }
        v.insert( v.end()
                , boost::begin(collapse_signature(source,wc))
                , boost::end(collapse_signature(source,wc))
                );
    } catch(...) {}
    return v;
}
////////////////////////////////////////////////////////////////////////////////

vector<indexed_token> rhs_from_string( string const& sss
                                     , indexed_token_factory& tf )
{
    return from_string(sss,tf);
    static regex rhs(" -> (.*?) ### ");

    smatch what;
    regex_search(sss,what,rhs);
    string str = what.str(1);

    return from_string(str,tf);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace xrsdb

