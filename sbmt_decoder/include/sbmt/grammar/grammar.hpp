#ifndef SBMT_GRAMMAR_GRAMMAR_HPP
#define SBMT_GRAMMAR_GRAMMAR_HPP

#include <sbmt/logmath.hpp>
#include <sbmt/token/indexed_token.hpp>
#include <sbmt/grammar/syntax_rule.hpp>
#include <sbmt/printer.hpp>
#include <sbmt/grammar/syntax_id_type.hpp>
#include <sbmt/grammar/features_byid.hpp>
#include <sbmt/feature/feature_vector.hpp>
#include <sbmt/grammar/text_features_byid.hpp>
#include <boost/foreach.hpp>
namespace sbmt {
////////////////////////////////////////////////////////////////////////////////
///
/// \file grammar.hpp
///
/// if you ever have a function or method that calculates something about
/// a grammar or rule, ask yourself: "can i write the function in terms of
/// the existing grammar class interface".  if you can, DO NOT MAKE IT A
/// MEMBER OF GRAMMAR.  thats because I anticipate at least two or three other
/// types of grammar implementations, and the fewer functions we have to 
/// duplicate, the better --Michael
///
////////////////////////////////////////////////////////////////////////////////

/// grammars must support a grammar_rule_id -> grammar::rule_type (binary rule), e.g. indexed_syntax_rule
///FIXME: make this unsigned (32 bit) index into grammar's vector/multiindex
typedef void *grammar_rule_id;

static const grammar_rule_id NULL_GRAMMAR_RULE_ID=0;

template <class GT>
score_t pessimistic_rule_score(GT const& gram, typename GT::rule_type const& r)
{
    return gram.rule_score(r) * gram.rule_score_estimate(r);
}

////////////////////////////////////////////////////////////////////////////////

template <class S, class T, class GT>
void print( std::basic_ostream<S,T>& os
          , typename GT::rule_type r
          , GT const& gram )
{
    token_label::saver sav(os);
    os << token_label(gram.dict());
    if (gram.is_complete_rule(r)) {
        os << "X: " << gram.get_syntax(r) << '\n';
    }
    os << "V: " << gram.rule_lhs(r) << " -> ";
    for (std::size_t x = 0; x != gram.rule_rhs_size(r); ++x) {
        if (is_lexical(gram.rule_rhs(r,x))) {
            os << '"' << gram.rule_rhs(r,x)<< "\" ";
        }
        else os << gram.rule_rhs(r,x) << ' ';
    }
    os << "###";
    if (gram.is_complete_rule(r)) os << " id=" << gram.get_syntax(r).id();
    std::set<feature_id_type> score_names;
    BOOST_FOREACH(feature_vector::value_type const& sc, gram.rule_scores(r))
    {
        os << ' ' << gram.feature_names().get_token(sc.first) << '=' << sc.second;
        score_names.insert(sc.first);
    }
    
    BOOST_FOREACH(text_byid_type tc, gram.rule_text(r))
    {
        if (score_names.find(tc.first) == score_names.end()) {
            os << ' ' << gram.feature_names().get_token(tc.first) 
                      << "={{{" << tc.second << "}}}";
        }
    }
}
    

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

//#include <sbmt/grammar/grammar_in_memory.hpp>

#endif // SBMT_GRAMMAR_GRAMMAR_HPP
