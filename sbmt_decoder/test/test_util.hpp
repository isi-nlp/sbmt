#ifndef   SBMT_TEST_TEST_UTIL_HPP
#define   SBMT_TEST_TEST_UTIL_HPP

#define DEBUG
#include <graehl/shared/debugprint.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <iostream>
#include <string>
#include <sbmt/grammar/syntax_rule.hpp>
#include <graehl/shared/epsilon.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
/// mixed equality between tree_nodes and tokens and rule_nodes for testing.
/// not sure it should be interface code, so only including in test file
///
////////////////////////////////////////////////////////////////////////////////
template <class T>
bool operator==(typename syntax_rule<T>::tree_node const& n, token<T> const& t)
{
    return n.get_token() == t;
}

template <class T>
bool operator==(token<T> const& t, typename syntax_rule<T>::tree_node const& n)
{
    return n == t;
}

template <class T>
bool operator!=(typename syntax_rule<T>::tree_node const& n, token<T> const& t)
{
    return !(n == t);
}

template <class T>
bool operator!=(token<T> const& t, typename syntax_rule<T>::tree_node const& n)
{
    return n != t;
}

template <class ChT,class TrT>
std::basic_ostream<ChT,TrT>& 
operator << ( std::basic_ostream<ChT,TrT>& os
            , typename syntax_rule<fat_token>::tree_node const& n) 
{
    return os << n.get_token();
}

template <class ChT,class TrT, class T>
std::basic_ostream<ChT,TrT>& 
operator << ( std::basic_ostream<ChT,TrT>& os
            , typename syntax_rule<T>::tree_node const& n) 
{
    return os << n.get_token();
}

} // namespace sbmt

template <class T, class TokenFactory>
std::string node_to_string(typename sbmt::syntax_rule<T>::tree_node const& n
                          ,TokenFactory& tf)
{
    std::stringstream str;
    str << print(n,tf);
    
    return str.str();
}

template <class T, class TF>
std::string node_to_string( typename sbmt::syntax_rule<T>::rule_node const& n
                          , TF& tf)
{
    std::stringstream str;
    str << print(n,tf);
    
    return str.str();
}

template <class T, class TF>
void to_string_recurse( std::string& str
                      , typename sbmt::syntax_rule<T>::tree_node const& n
                      , TF& tf );

template <class T, class TF>
void to_string_recurse( std::string& str
                      , typename sbmt::syntax_rule<T>::tree_node const& n
                      , TF& tf )
{
    BOOST_CHECKPOINT("to_string_recurse begin");
    str += node_to_string<T,TF>(n,tf);
    
    typename sbmt::syntax_rule<T>::lhs_children_iterator 
        itr = n.children_begin();
    typename sbmt::syntax_rule<T>::lhs_children_iterator 
        end = n.children_end();
    
    if (itr != end) {
        str += "(";
        to_string_recurse<T,TF>(str, *itr, tf);
        ++itr;
        for (; itr != end; ++itr) {
            str += " ";
            to_string_recurse<T,TF>(str, *itr, tf);
        }
        str += ")";
    }
    BOOST_CHECKPOINT("to_string_recurse end");
}

template <class T, class TF>
std::string to_string(sbmt::syntax_rule<T> const& r, TF tf)
{
    BOOST_CHECKPOINT("to_string begin");
    
    std::stringstream str;
    str << print(r,tf);

    return str.str();
}

#define LOGNUMBER_CHECK_CLOSE_E(A,B,C,E)                \
{                                                      \
 std::stringstream sstr;                               \
 sstr << "values " << (A) << " and " << (B)            \
     << " are not within tolerance " << (C);           \
 BOOST_CHECK_MESSAGE(E,(sstr.str())); \
} //(A).difference(B)/(A) < (C)

#define LOGNUMBER_CHECK_CLOSE(A,B,C) LOGNUMBER_CHECK_CLOSE_E(A,B,C,(A)==(B) || (A).nearly_equal((B), (C)))

#define LOGNUMBER_CHECK_EXP_CLOSE(A,B,C) LOGNUMBER_CHECK_CLOSE_E(A,B,C,graehl::very_close((A).log(),(B).log(),(C.linear())))


#define LOGNUMBER_CHECK_CLOSE_E_MSG(MSG,A,B,C,E)       \
{                                                      \
 std::stringstream sstr;                               \
 sstr << MSG << "values " << (A) << " and " << (B)            \
     << " are not within tolerance " << (C);           \
 BOOST_CHECK_MESSAGE(E,(sstr.str())); \
} //(A).difference(B)/(A) < (C)

#define LOGNUMBER_CHECK_EXP_CLOSE_MSG(MSG,A,B,C) LOGNUMBER_CHECK_CLOSE_E_MSG(MSG,A,B,C,graehl::very_close((A).log(),(B).log(),(C.linear())))

#define LOGNUMBER_CHECK_CLOSE(A,B,C) LOGNUMBER_CHECK_CLOSE_E(A,B,C,(A)==(B) || (A).nearly_equal((B), (C)))

#define LOGNUMBER_CHECK_(A,B,C)                   \

#define LOGNUMBER_CHECK_EQ(A,B) LOGNUMBER_CHECK_CLOSE(A,B,tolerance)

#endif // SBMT_TEST_TEST_UTIL_HPP
