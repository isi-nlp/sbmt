#include "word_context_info.hpp"
#include "boost/foreach.hpp"

namespace word_context {

vector<indexed_token> 
word_context_info_factory::
//get_lhs_yield(const grammar_in_mem::rule_type& rule)  const
get_lhs_yield(const indexed_syntax_rule& rule)  const
{
    vector<indexed_token> v;
    indexed_syntax_rule :: lhs_preorder_iterator it;
    for(it = rule.lhs_begin(); it!=rule.lhs_end();++it){
        if(it->is_leaf()){v.push_back(it->get_token());}
    }
    return v;
}
} // namespace

