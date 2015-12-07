# if ! defined(SBMT__GRAMMAR__TREE_TAGS_HPP)
# define       SBMT__GRAMMAR__TREE_TAGS_HPP

# include <boost/tokenizer.hpp>
# include <boost/regex.hpp>
# include <string>
# include <stack>
# include <vector>
 #include <RuleReader/Rule.h>
 #include <RuleReader/RuleNode.h>

namespace sbmt {

// lisp/Tiburon format: S(NP(dogs) VB(lie) ) - note: ') )' space matters
////////////////////////////////////////////////////////////////////////////////


// the tokens are typed because the lmstring is parsed for " " vs. ( ) (/ ) and typed

template <class TF>
typename TF::token_t
tag_or_top(std::string const& s,TF &tf,char const* TOP="TOP") 
{
    return tf.native_word("[" + s + "]");
}

template <class TF>
typename TF::token_t
close_tag(std::string const& s,TF &tf) 
{
        return tf.native_word("[/" + s + "]");
}


template <class OutT, class TF>
void lisp_tree_tags(std::string const& str, OutT out, TF& tf)
{
    using namespace boost;
    using std::string;
    using std::stack;
    
    regex wsp("\\s+");
    sregex_token_iterator i = make_regex_token_iterator(str,wsp,-1);
    sregex_token_iterator e;
    
    regex begintree("^\\((\\S+)$");
    regex lexitems("^(\\S+)\\)$");
    regex whole("^((\\S+)\\)|\\((\\S+)|\\))$");
    
    stack<string> stk;
    for (; i != e; ++i) {
        smatch m;
        std::string sm(i->str());
        //std::clog << '"' << sm << '"' << ' ';
        if (regex_match(sm,m,lexitems)) {
            std::string s(m.str(1));
            //std::clog << "[lex " << m.str(1) <<":"<<s<<":" << m.size() << "] ";
            *out = tf.native_word(s);
            ++out;
            *out = close_tag(stk.top(),tf);
            ++out;
            //std::clog << "[end " << stk.top() << "] ";
            stk.pop();
        } else if (regex_match(sm,m,begintree)) {
            std::string s(m.str(1));
            //std::clog << "[begin " << m.str(1) <<":"<<s<<":" << m.size() << "] ";
            stk.push(s);
            *out = tag_or_top(s,tf);
            ++out;
        } else if (sm == ")") {
            //std::clog << "[end " << stk.top() << "] ";
            *out = close_tag(stk.top(),tf);
            ++out;
            stk.pop();
        } else {
            //stk.push(sm);
            *out = tf.native_word(sm);
            ++out;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class TF>
std::vector<typename TF::token_t>
lisp_tree_tags(std::string const& str, TF& tf)
{
    std::vector<typename TF::token_t> v;
    lisp_tree_tags(str,std::back_inserter(v),tf);
    return v; // nvro
}

////////////////////////////////////////////////////////////////////////////////

// syntax-rule format: S(NP("dogs") VB("lie"))
////////////////////////////////////////////////////////////////////////////////

typedef std::runtime_error tree_tag_error;

template <class OutT, class TF>
void tree_tags(ns_RuleReader::RuleNode &n, OutT out, TF& tf)
{
    using std::string;
    using ns_RuleReader::RuleNode;
    string const& lab=n.getString(false,false);
    if (n.isNonTerminal()) {
        
        *out=tag_or_top(lab,tf);
        ++out;
        
        typedef RuleNode::children_type C;
        C const& c=*n.getChildren();
        for (C::const_iterator i=c.begin(),e=c.end();i!=e;++i)
            tree_tags(**i,out,tf);
        
        *out=close_tag(lab,tf);
        ++out;
    } else if (n.isLexical()) {
        *out=tf.native_word(lab);
        ++out;
    } else {
        throw tree_tag_error("tree_tag had syntax-rule variable node");
    }
}


template <class OutT, class TF>
void tree_tags(std::string const& str, OutT out, TF& tf)
{
    ns_RuleReader::Rule r(str+" -> \"f\" ### id=1");

//    RuleNode *n=r.getLHSRoot();
    tree_tags(*r.getLHSRoot(),out,tf);
}


////////////////////////////////////////////////////////////////////////////////

template <class TF>
std::vector<typename TF::token_t>
tree_tags(std::string const& str, TF& tf)
{
    std::vector<typename TF::token_t> v;
    tree_tags(str,std::back_inserter(v),tf);
    return v; // nvro
}

////////////////////////////////////////////////////////////////////////////////

}

# endif //     SBMT__GRAMMAR__TREE_TAGS_HPP

